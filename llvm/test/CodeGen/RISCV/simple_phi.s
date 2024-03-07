	.text
	.file	"mytest.cpp"
	.globl	_Z3bars                         # -- Begin function _Z3bars
	.p2align	4, 0x90
	.type	_Z3bars,@function
_Z3bars:                                # @_Z3bars
	.cfi_startproc
# %bb.0:                                # %entry
	pushq	%rbp
	.cfi_def_cfa_offset 16
	pushq	%r15
	.cfi_def_cfa_offset 24
	pushq	%r14
	.cfi_def_cfa_offset 32
	pushq	%rbx
	.cfi_def_cfa_offset 40
	pushq	%rax
	.cfi_def_cfa_offset 48
	.cfi_offset %rbx, -40
	.cfi_offset %r14, -32
	.cfi_offset %r15, -24
	.cfi_offset %rbp, -16
	movslq	%edi, %rbx
	cmpl	$-529, %edi                     # imm = 0xFDEF
	jl	.LBB0_6
# %bb.1:                                # %for.body.lr.ph
	movl	%edi, %ebp
	movl	$-529, %r14d                    # imm = 0xFDEF
	cmovgl	%edi, %r14d
	addl	$530, %r14d                     # imm = 0x212
	shlq	$2, %r14
	xorl	%r15d, %r15d
	jmp	.LBB0_2
	.p2align	4, 0x90
.LBB0_4:                                # %if.else
                                        #   in Loop: Header=BB0_2 Depth=1
	callq	_Z3foov@PLT
	addl	%eax, %eax
	movl	_ZL7Numbers(%r15), %ecx
	addl	%ebp, %ecx
	subl	%ecx, %eax
.LBB0_5:                                # %for.inc
                                        #   in Loop: Header=BB0_2 Depth=1
	cltq
	addq	%rax, %rbx
	addq	$4, %r15
	cmpq	%r15, %r14
	je	.LBB0_6
.LBB0_2:                                # %for.body
                                        # =>This Inner Loop Header: Depth=1
	cmpw	$119, %bp
	jg	.LBB0_4
# %bb.3:                                # %if.then
                                        #   in Loop: Header=BB0_2 Depth=1
	movl	_ZL7Numbers(%r15), %eax
	addl	%ebp, %eax
	jmp	.LBB0_5
.LBB0_6:                                # %for.cond.cleanup
	addq	$5, %rbx
	movq	%rbx, %rax
	addq	$8, %rsp
	.cfi_def_cfa_offset 40
	popq	%rbx
	.cfi_def_cfa_offset 32
	popq	%r14
	.cfi_def_cfa_offset 24
	popq	%r15
	.cfi_def_cfa_offset 16
	popq	%rbp
	.cfi_def_cfa_offset 8
	retq
.Lfunc_end0:
	.size	_Z3bars, .Lfunc_end0-_Z3bars
	.cfi_endproc
                                        # -- End function
	.type	_ZL7Numbers,@object             # @_ZL7Numbers
	.section	.rodata,"a",@progbits
	.p2align	4, 0x0
_ZL7Numbers:
	.long	5                               # 0x5
	.long	20                              # 0x14
	.long	70                              # 0x46
	.long	164                             # 0xa4
	.long	320                             # 0x140
	.long	25                              # 0x19
	.long	555                             # 0x22b
	.size	_ZL7Numbers, 28

	.ident	"clang version 16.0.6 (git@github.com:karouzakisp/llvm-project.git d64e19dfd056458bb782c4e6af596f2b08a64998)"
	.section	".note.GNU-stack","",@progbits
