# Context

Many redundant sext and zext instructions still escape LLVM IR middle end optimizations. This is because it's quite complex to remove them. Other passes such as InstCombine and the DAGCombiner are local and don't work across PHI cycles. Our approach removes many of them by using fillType aware analysis together with a greatest fixpoint analysis. The function pass achieves code size reductions up to 7.1% in the Oz pipeline (LLVM test-suite) in initial evaluation; optimization remarks may help identify additional missed opportunities and guide future decisions.

An earlier version of this work was presented at EuroLLVM 2024 in the talk
[Sign Extension Optimizations inside LLVM](https://www.youtube.com/watch?v=HeIDxixC-VM).

The EuroLLVM talk focused mostly on the fillType analysis, corresponding to the
first component described below. Since then, the work has been extended into
the complete four-component design proposed in this RFC.

I also should note that this is one of my first RFCs so I'd appreciate some feedback.

## Proposed Solution

I've implemented a holistic solution that resulted in an sign/zero extension removal pass named WIA in short. The pass is partly based on the paper Widening Integer Arithmetic, Redwine and Ramsey, CC 2004. The paper is referenced in the CodeGen documentation of LLVM.

## Design

I've split the design into 4 classes.

The first class **WideningIntegerArithmetic** creates solutions that hold all possible
widths for a given instruction I. By width we mean the bitwidth of I's type.
A solution holds some other data too, such as the *fillType* of I. This allows us to drop many extensions.

We use the CC paper here, its type system and the meaning of the fillTypes, to create the necessary solutions for each Instruction.

A *fillType* indicates for an operand what it produces and accepts in its upper bits.

An operand has a fillType only if it is _widenable_, i.e, if by applying the operator to a _wider_ value, we can _simulate_ the operator applied to _narrow_ values.

wide value:    [ fillType bits ][ original bits ]

SIGN:            [ b  b  b  b  b  b  b ][ b  .  .  .  .  .  .  . ]
ZEROS:         [ 0  0  0  0  0  0  0  ][ .  .  .  .  .  .  .  . ]
ANYTHING:  [ ?  ?  ?  ?  ?  ?  ?  ?  ][ .  .  .  .  .  .  .  . ]
                                                        ^
                                                   sign bit

For example since the OR instruction can produce a ZERO when operands are zero-filled, 8-bit OR can be implemented as a 16-bit OR.

x narrow:          1010 0101
y narrow:          0001 1000
result narrow:  <u>1011 1101</u>

zext(x) to i16:    0000 0000 1010 0101
zext(y) to i16:    0000 0000 0001 1000
                   -------------------
or result i16:     0000 0000 <u>1011 1101</u>

This clearly explains why the low bits do not change, if we have a fillType. Then we can simulate the *narrow* operator with a *wider* one.

Almost all integer instructions are *widenable*. The specific list is derived from the CC paper itself. Regarding overflowing operators, they are widenable only if we have the nuw and nsw flag and the fillType on both operands is ZERO if nuw or SIGN if nsw.

At the end each Instruction has some solutions to bigger widths, and we can use them if we want to promote to a bigger type.

The second class **ExtRewritePlanner** decides if it's profitable to remove an extension. It tries to build a *wide* or *narrow* expression to apply either *widening* or *narrowing*.

<u>Suppose we have a sext i8 **%a** to i32</u>.

We have two options:

1. We can either *narrow* all the *relevant* instructions around the extension to **i8** making the extension redundant. This is possible if the extended bits are not used downstream by any of the users of the extension.

2. We can try to promote all the *relevant* instructions to **i32** allowing us to drop the extension. This is explained more below.

By *relevant* instructions we mean what is reachable through operands and users around the extension to form the mathematical expression that the code tries to calculate.

The first option is an ad-hoc analysis similar to the MIR pass that exists in the backend of RISC-V named RISCVOptWInstrs.cpp.

The second option is legal if the source value (**%a** of the extension) has a solution at the extension's destination width with a fillType that satisfies the extension semantics. For zext we need fillType of ZEROS, and for sext we need a fillType of SIGN. Also, all the relevant instructions must have a *fillType* that is needed around the expression. This forms a lattice of fillTypes on what operands can provide as a fillType and what fillType an instruction can produce to its users. This is applicable much more often than the first option because in most cases the extended bits are demanded (used downstream).

The third class **PromoteConstraintSolver** is used only when we are *widening* and checks if the promoted expression graph is legal based on the fillType constraints of the promoted expression graph. It uses a greatest fixpoint analysis that is needed especially when we have PHI cycles, because one solution depends on another and the reverse; a simple approach doesn't work here. Conceptually, it uses a finite lattice where each node maps to set of possible promote states `(width, fillType)`. The solver starts at the top element, containing all candidate fillTypes at the promoted width, and iteratively removes states that violate legal operand/user constraints.
If the fixpoint does not end up to an empty state set (each node retains at least one valid state) and we have found a matching fill (ZERO for `zext` SIGN for `sext`) then the promoted graph is legal.

The fourth class, named **Rebuilder** just does all the necessary rewrites to remove the extension, and it tries to preserve nuw/nsw flags (future work).

## Examples

I will focus on examples that can't be easily fixed by other passes and show the real value of this pass.

To keep the text small, I'll show C code on the examples and minimal IR diffs if needed.

### Example 1

 The following example is from the LLVM Test Suite benchmark
`MultiSource/Benchmarks/MiBench/automotive-susan/automotive-susan.test`, in `susan.c`:

```c
int n=0;
for (i=5;i<y_size-5;i++) {
  for (j=5;j<x_size-5;j++) {
    x = r[i*x_size+j];
    if (x>0) {
      if ( /* condition */ ) {
        corner_list[n].info=0;
        corner_list[n].x=j;
        corner_list[n].y=i;
        corner_list[n].dx=cgx[i*x_size+j];
        corner_list[n].dy=cgy[i*x_size+j];
        corner_list[n].I=in[i*x_size+j];
        n++;
        if(n==MAX_CORNERS) exit(1);
      }
    }
  }
}
```

On this C code, WIA promotes n through nested-loop conditional increments, removing repeated sexts feeding corner_list[n]. Other passes such as InstCombine, TypePromotion, CodeGenPrepare and DAGCombiner cannot handle those cases and are not easily fixed.

A small IR diff looks like this.

```
- %n.02190.us = phi i32 [ 0, ... ], [ %n.2.us, ... ]
- %n.12186.us = phi i32 [ %n.02190.us, ... ], [ %n.2.us, ... ]
- %idxprom1343.us = sext i32 %n.12186.us to i64
- %arrayidx1344.us = getelementptr inbounds [24 x i8], ptr %corner_list, i64 %idxprom1343.us
- %inc1370.us = add nsw i32 %n.12186.us, 1
- %cmp1371.us = icmp eq i32 %inc1370.us, 15000
- %n.2.us = phi i32 [ %inc1370.us, %if.then1342.us ], [ %n.12186.us, ... ]

+ %n.02190.us = phi i64 [ 0, ... ], [ %n.2.us, ... ]
+ %n.12186.us = phi i64 [ %n.02190.us, ... ], [ %n.2.us, ... ]
+ %arrayidx1344.us = getelementptr inbounds [24 x i8], ptr %corner_list, i64 %n.12186.us
+ %inc1370.us = add nsw i64 %n.12186.us, 1
+ %cmp1371.us = icmp eq i64 %inc1370.us, 15000
+ %n.2.us = phi i64 [ %inc1370.us, %if.then1342.us ], [ %n.12186.us, ... ]
```

### Example 2

The following example is from the LLVM Test Suite benchmark
`MultiSource/Applications/d`, in `parse.c`:

```c
int i, j, head = 0, tail = 0;
SNode **q = MALLOC(ERROR_RECOVERY_QUEUE_SIZE * sizeof(SNode*));

for (sn = p->snode_hash.last_all; sn; sn = sn->all_next) {
  if (tail < ERROR_RECOVERY_QUEUE_SIZE - 1)
    q[tail++] = sn;
}

while (tail > head) {
  sn = q[head++];
  ...
  if (tail < ERROR_RECOVERY_QUEUE_SIZE - 1)
    q[tail++] = sn->zns.v[i]->sns.v[j];
}
```

WIA promotes the  loop carried `tail` from `i32` to `i64`, through a loop removing repeated `sext` used for `q[tail++]`. This is not easy for other passes such as InstCombine or DAGCombiner because the rewrite requires changing multiple loop-carried PHIs, conditional increments and GEP Indices.

A small IR diff looks like this

```
- %tail.0480.i = phi i32 [ %tail.1.i, %for.inc.i ], [ 0, ... ]
- %cmp21.i = icmp slt i32 %tail.0480.i, 9999
- %inc23.i = add nsw i32 %tail.0480.i, 1
- %idxprom.i = sext i32 %tail.0480.i to i64
- %arrayidx.i = getelementptr inbounds [8 x i8], ptr %call.i196, i64 %idxprom.i
- %tail.1.i = phi i32 [ %inc23.i, %if.then22.i ], [ %tail.0480.i, ... ]
- %tail.3.lcssa.i = phi i32 [ %tail.2495.i, ... ], [ %tail.6.i, ... ]
- %39 = sext i32 %tail.3.lcssa.i to i64
- %cmp29.i = icmp slt i64 %indvars.iv.next526.i, %39

+ %tail.0480.i = phi i64 [ %tail.1.i, %for.inc.i ], [ 0, ... ]
+ %cmp21.i = icmp slt i64 %tail.0480.i, 9999
+ %inc23.i = add nsw i64 %tail.0480.i, 1
+ %arrayidx.i = getelementptr inbounds [8 x i8], ptr %call.i196, i64 %tail.0480.i
+ %tail.1.i = phi i64 [ %inc23.i, %if.then22.i ], [ %tail.0480.i, ... ]
+ %tail.3.lcssa.i = phi i64 [ %tail.2495.i, ... ], [ %tail.6.i, ... ]
+ %cmp29.i = icmp slt i64 %indvars.iv.next526.i, %tail.3.lcssa.i
```

### Example 3

The following example is from the LLVM Test Suite benchmark `MultiSource/Benchmarks/Ptrdist/bc`, in `scan.c`:

```c
ch = charMap[*p];
while (check[base[state] + ch] != state) {
  state = fallback[state];
  if (state >= 144)
   ch = meta[ch];
}
state = next[base[state] + ch];
```

On this C code, WIA promotes ch PHIs used in GEP indices. This moves the extensions to PHI incoming values and removes repeated extensions from the loop. Other passes such as InstCombine and the DAGCombiner cannot easily handle those cases.

```
charMap = internal constant [256 x i8] ...
@meta = internal constant [53 x i8] ...
@check = internal constant [247 x i16] ...
@next = internal constant [247 x i16] ...

%ch.ptr = getelementptr i8, ptr @charMap, i64 %char
%ch = load i8, ptr %ch.ptr
+ %ch64 = sext i8 %ch to i64

- %c = phi i8 [ %ch, %entry ], [ 1, %loop ]
- %c64 = sext i8 %c to i64
+ %c64 = phi i64 [ %ch64, %entry ], [ 1, %loop ]

%check.idx = add i64 %base64, %c64
%check.ptr = getelementptr [2 x i8], ptr @check, i64 %check.idx

%meta.ptr = getelementptr i8, ptr @meta, i64 %c64
%meta.ch = load i8, ptr %meta.ptr
+ %meta64 = zext i8 %meta.ch to i64

- %c.next = phi i8 [ %meta.ch, %fallback ], [ %c, %retry ]
- %c.next64 = sext i8 %c.next to i64
+ %c.next64 = phi i64 [ %meta64, %fallback ], [ %c64, %retry ]

%next.idx = add i64 %base.next64, %c.next64
%next.ptr = getelementptr [2 x i8], ptr @next, i64 %next.idx
```

base64 and base.next64 are coming from other GEPS but due to lack of space here, I won’t show them.

## Why Existing Passes are Insufficient

The fillType analysis itself doesn't make much sense to be integrated into passes, because it is quite heavy.

Also, for the motivating examples, existing passes are insufficient and not easy to fix for many reasons.

InstCombine is local and doesn't support cyclic phis. This requires either SCCs or a greatest fixpoint analysis to be implemented and this is not trivial.

DAGCombiner works inside phi boundaries and it doesn't work across PHI nodes.

TypePromotion works for a specific use case around icmps and it also supports promoting phis only with zext, but the zext must be inside a loop. It's not trivial to add sext support in the way the pass is designed.

Handling extensions inside Loops with CodeGenPrepare isn't trivial too, it requires many things: promoting through recurrences, legality of the promotions, a better profitability model and maybe more things.

## Code-size results

I run the llvm test suite with the Oz pipeline, on a X86 machine.
Several benchmarks show code size reductions up to 7.1% while the largest observed regression is limited to 0.4%

| Metric               | Change    |
| -------------------- | ---------:|
| Geomean              | 0.000067% |
| Largest improvement  | 7.1%      |
| Largest regression   | 0.4%      |
| Benchmarks evaluated | 2515      |

### Results for the motivating examples

| Source example | LLVM Test Suite benchmark        | WIA size | Upstream size | Bytes saved | Reduction |
| -------------- | -------------------------------- | --------:| -------------:| -----------:| ---------:|
| `susan.c`      | `MiBench-automotive-susan/susan` | 27,992   | 28,088        | 96 B        | 0.3%      |
| `parse.c`      | `Applications/d`                 | 87,551   | 87903         | 352 B       | 0.4%      |
| `scan.c`       | `/Ptrdist/bc`                    | 46,164   | 46,372        | 208 B       | 0.45%     |

### LLVM Test suite code-size improvements

The table includes some llvm-test-suite benchmarks ran at the Oz optimization level using llvm test-suite.

| Benchmark                            | august4_wia_v1 | august4_upstream_v1 | Bytes Saved | Diff |
| ------------------------------------ | --------------:| -------------------:| -----------:| ----:|
| mason                                | 2,253.00       | 2,413.00            | 160         | 7.1% |
| GCC-C-execute-930603-3               | 280.00         | 288.00              | 8           | 2.9% |
| GCC-C-execute-20021010-2             | 301.00         | 309.00              | 8           | 2.7% |
| GCC-C-execute-pr52209                | 270.00         | 272.00              | 2           | 0.7% |
| GCC-C-execute-pr70586                | 412.00         | 414.00              | 2           | 0.5% |
| GCC-C-execute-pr66556                | 426.00         | 428.00              | 2           | 0.5% |
| bc                                   | 46,164.00      | 46,372.00           | 208         | 0.5% |
| telecomm-gsm                         | 44,508.00      | 44,700.00           | 192         | 0.4% |
| toast                                | 44,524.00      | 44,716.00           | 192         | 0.4% |
| agrep                                | 56,173.00      | 56,413.00           | 240         | 0.4% |
| make_dparser                         | 87,551.00      | 87,903.00           | 352         | 0.4% |
| is                                   | 4,615.00       | 4,631.00            | 16          | 0.3% |
| automotive-susan                     | 27,992.00      | 28,088.00           | 96          | 0.3% |
| GCC-C-execute-mode-dependent-address | 1,033.00       | 1,036.00            | 3           | 0.3% |
| XSBench                              | 12,925.00      | 12,957.00           | 32          | 0.2% |
| paq8p                                | 100,910.00     | 101,150.00          | 240         | 0.2% |
| minisat                              | 21,518.00      | 21,566.00           | 48          | 0.2% |
| Shootout-strcat                      | 533.00         | 534.00              | 1           | 0.2% |
| IRSmk                                | 8,824.00       | 8,840.00            | 16          | 0.2% |
| ReedSolomon                          | 10,195.00      | 10,211.00           | 16          | 0.2% |
| sqlite3                              | 488,863.00     | 489,551.00          | 688         | 0.1% |
| lemon                                | 47,534.00      | 47,566.00           | 32          | 0.1% |
| office-ispell                        | 51,206.00      | 51,238.00           | 32          | 0.1% |
| lencod                               | 749,896.00     | 750,280.00          | 384         | 0.1% |
| hbd                                  | 32,559.00      | 32,575.00           | 16          | 0.0% |
| SPASS                                | 505,554.00     | 505,794.00          | 240         | 0.0% |
| pairlocalalign                       | 455,394.00     | 455,602.00          | 208         | 0.0% |

Lower values are better. Complete results, including other changes can be provided if requested.

### All Regressions (.text)

### LLVM Test Suite regressions

The following table shows all the code-size regressions observed with WIA at the
`-Oz` level. The code size regressions occur mainly because of promotions to the i64 type. On X86 wider operations can require larger encodings. The current X86 TTI cost model does not account for instruction encoding size. I spent some time trying to fix the regressions, below are all the remaining unfixed ones.

| Benchmark                | august4_wia_v1 | august4_upstream_v1 | Bytes Added | Diff  |
| ------------------------ | --------------:| -------------------:| -----------:| -----:|
| GCC-C-execute-20000703-1 | 312.00         | 312.00              | 0           | 0.0%  |
| CLAMR                    | 732,720.00     | 732,704.00          | 16          | -0.0% |
| consumer-typeset         | 442,830.00     | 442,814.00          | 16          | -0.0% |
| kc                       | 396,253.00     | 396,237.00          | 16          | -0.0% |
| Packing-dbl              | 98,896.00      | 98,880.00           | 16          | -0.0% |
| gs                       | 152,897.00     | 152,849.00          | 48          | -0.0% |
| PENNANT                  | 112,537.00     | 112,489.00          | 48          | -0.0% |
| mybison                  | 66,391.00      | 66,359.00           | 32          | -0.0% |
| clamscan                 | 538,706.00     | 538,370.00          | 336         | -0.1% |
| HPCCG                    | 23,075.00      | 23,043.00           | 32          | -0.1% |
| Obsequi                  | 36,470.00      | 36,406.00           | 64          | -0.2% |
| miniAMR                  | 63,009.00      | 62,897.00           | 112         | -0.2% |
| AMGmk                    | 17,950.00      | 17,918.00           | 32          | -0.2% |
| archie                   | 17,563.00      | 17,531.00           | 32          | -0.2% |
| city                     | 7,752.00       | 7,736.00            | 16          | -0.2% |
| hexxagon                 | 15,098.00      | 15,066.00           | 32          | -0.2% |
| CoMD                     | 40,281.00      | 40,185.00           | 96          | -0.2% |
| fhourstones              | 6,914.00       | 6,897.00            | 17          | -0.2% |
| PathFinder               | 22,946.00      | 22,882.00           | 64          | -0.3% |
| ocean                    | 4,370.00       | 4,354.00            | 16          | -0.4% |

## Compile Time

In order to measure the compile time I used an idle machine doing 10 paired runs of the CTMark with the pass enabled and not enabled. The pass reduced the total CTMark compilation time by approximately 0.1%.

## Proposal

This RFC proposes a new middle end optimization for the Oz pipeline, that achieves code size reductions up to 7.1% with some regressions and no compilation time impact. Future work can include fixing the remaining regressions, increasing the coverage of the pass by using OptimizationRemarks, evaluating runtime impact on the Os pipeline, and enabling the pass for additional targets such as AArch64 or RISC-V.

I look forward to the feedback from the community.
