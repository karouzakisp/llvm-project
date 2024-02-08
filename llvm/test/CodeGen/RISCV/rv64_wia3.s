	.text
	.file	"rv64_wia3.ll"
	.globl	main                            # -- Begin function main
	.p2align	4, 0x90
	.type	main,@function
main:                                   # @main
	.cfi_startproc
# %bb.0:                                # %entry
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	subq	$32, %rsp
	movl	$0, -20(%rbp)
	xorl	%edi, %edi
	callq	time@PLT
	movl	%eax, %edi
	callq	srand@PLT
	callq	rand@PLT
	movl	$1000, %ecx                     # imm = 0x3E8
	cltd
	idivl	%ecx
	movl	%edx, -16(%rbp)
	callq	rand@PLT
	movl	$1000, %ecx                     # imm = 0x3E8
	cltd
	idivl	%ecx
	movl	%edx, -12(%rbp)
	movl	-16(%rbp), %eax
	xorl	-12(%rbp), %eax
	movl	%eax, -8(%rbp)
	movl	-8(%rbp), %eax
	andl	$24, %eax
	movw	%ax, -2(%rbp)
	movswl	-2(%rbp), %eax
	addq	$32, %rsp
	popq	%rbp
	.cfi_def_cfa %rsp, 8
	retq
.Lfunc_end0:
	.size	main, .Lfunc_end0-main
	.cfi_endproc
                                        # -- End function
	.ident	"clang version 16.0.6 (git@github.com:karouzakisp/llvm-project.git 15f6f65a06f37ce57594619d2a33222c522190af)"
	.section	".note.GNU-stack","",@progbits
