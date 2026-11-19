; RUN: llc -mtriple=riscv64 -O2 -print-after=widening-integer-arithmetic -o /dev/null < %s 2>&1 | FileCheck %s --check-prefix=LATE -D#REG_WIDTH=64
; RUN: llc -mtriple=riscv32 -O2 -print-after=widening-integer-arithmetic -o /dev/null < %s 2>&1 | FileCheck %s --check-prefix=LATE -D#REG_WIDTH=32
; RUN: llc -mtriple=riscv64 -O3 -print-after=widening-integer-arithmetic -o /dev/null < %s 2>&1 | FileCheck %s --check-prefix=LATE -D#REG_WIDTH=64
; RUN: llc -mtriple=riscv32 -O3 -print-after=widening-integer-arithmetic -o /dev/null < %s 2>&1 | FileCheck %s --check-prefix=LATE -D#REG_WIDTH=32
; RUN: llc -mtriple=riscv64 -O2 -disable-wia -debug-pass=Structure -o /dev/null < %s 2>&1 | FileCheck %s --check-prefix=DISABLED --implicit-check-not=WideningIntegerArithmeticLegacy
; RUN: llc -mtriple=riscv32 -O2 -disable-wia -debug-pass=Structure -o /dev/null < %s 2>&1 | FileCheck %s --check-prefix=DISABLED --implicit-check-not=WideningIntegerArithmeticLegacy
; RUN: llc -mtriple=riscv64 -O0 -debug-pass=Structure -o /dev/null < %s 2>&1 | FileCheck %s --check-prefix=DISABLED --implicit-check-not=WideningIntegerArithmeticLegacy
; RUN: llc -mtriple=riscv32 -O0 -debug-pass=Structure -o /dev/null < %s 2>&1 | FileCheck %s --check-prefix=DISABLED --implicit-check-not=WideningIntegerArithmeticLegacy
; RUN: opt -mtriple=riscv64 -passes=wia,verify -S < %s | FileCheck %s --check-prefix=EXPLICIT
; RUN: opt -mtriple=riscv64 -passes='default<O2>' -print-after=wia -disable-output < %s 2>&1 | FileCheck %s --check-prefix=LATE -D#REG_WIDTH=64

; Late WIA is scheduled on RV32 and RV64; early WIA is scheduled on RV64.
; Both automatic invocations transform only minsize functions. An explicit
; new-PM invocation also transforms normal and optsize functions.
; DISABLED: Target Library Information

define i1 @minsize_cmp(ptr %p) minsize {
; LATE-LABEL: define i1 @minsize_cmp(
; LATE:       [[A:%.*]] = load i8, ptr %p, align 1
; LATE-NEXT:  [[EXT:%.*]] = sext i8 [[A]] to i[[#REG_WIDTH]]
; LATE-NEXT:  [[ADD:%.*]] = add i[[#REG_WIDTH]] [[EXT]], 79
; LATE-NEXT:  [[CMP:%.*]] = icmp ult i[[#REG_WIDTH]] [[ADD]], 131
; LATE-NEXT:  ret i1 [[CMP]]
; EXPLICIT-LABEL: define i1 @minsize_cmp(
; EXPLICIT:       [[A:%.*]] = load i8, ptr %p, align 1
; EXPLICIT-NEXT:  [[EXT:%.*]] = sext i8 [[A]] to i64
; EXPLICIT-NEXT:  [[ADD:%.*]] = add i64 [[EXT]], 79
; EXPLICIT-NEXT:  [[CMP:%.*]] = icmp ult i64 [[ADD]], 131
; EXPLICIT-NEXT:  ret i1 [[CMP]]
entry:
  %a = load i8, ptr %p, align 1
  %wrap = add i8 %a, -49
  %cmp = icmp slt i8 %wrap, 3
  ret i1 %cmp
}

define i1 @normal_cmp(ptr %p) {
; LATE-LABEL: define i1 @normal_cmp(
; LATE:       [[A:%.*]] = load i8, ptr %p, align 1
; LATE-NEXT:  [[ADD:%.*]] = add i8 [[A]], -49
; LATE-NEXT:  [[CMP:%.*]] = icmp slt i8 [[ADD]], 3
; LATE-NEXT:  ret i1 [[CMP]]
; EXPLICIT-LABEL: define i1 @normal_cmp(
; EXPLICIT:       [[A:%.*]] = load i8, ptr %p, align 1
; EXPLICIT-NEXT:  [[EXT:%.*]] = sext i8 [[A]] to i64
; EXPLICIT-NEXT:  [[ADD:%.*]] = add i64 [[EXT]], 79
; EXPLICIT-NEXT:  [[CMP:%.*]] = icmp ult i64 [[ADD]], 131
; EXPLICIT-NEXT:  ret i1 [[CMP]]
entry:
  %a = load i8, ptr %p, align 1
  %wrap = add i8 %a, -49
  %cmp = icmp slt i8 %wrap, 3
  ret i1 %cmp
}

define i1 @optsize_cmp(ptr %p) optsize {
; LATE-LABEL: define i1 @optsize_cmp(
; LATE:       [[A:%.*]] = load i8, ptr %p, align 1
; LATE-NEXT:  [[ADD:%.*]] = add i8 [[A]], -49
; LATE-NEXT:  [[CMP:%.*]] = icmp slt i8 [[ADD]], 3
; LATE-NEXT:  ret i1 [[CMP]]
; EXPLICIT-LABEL: define i1 @optsize_cmp(
; EXPLICIT:       [[A:%.*]] = load i8, ptr %p, align 1
; EXPLICIT-NEXT:  [[EXT:%.*]] = sext i8 [[A]] to i64
; EXPLICIT-NEXT:  [[ADD:%.*]] = add i64 [[EXT]], 79
; EXPLICIT-NEXT:  [[CMP:%.*]] = icmp ult i64 [[ADD]], 131
; EXPLICIT-NEXT:  ret i1 [[CMP]]
entry:
  %a = load i8, ptr %p, align 1
  %wrap = add i8 %a, -49
  %cmp = icmp slt i8 %wrap, 3
  ret i1 %cmp
}
