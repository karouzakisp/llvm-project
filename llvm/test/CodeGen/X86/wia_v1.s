	.text
	.file	"wia_v1.ll"
	.globl	foo                             # -- Begin function foo
	.p2align	4, 0x90
	.type	foo,@function
foo:                                    # @foo
# %bb.0:
	movq	gd@GOTPCREL(%rip), %rax
	xorw	(%rax), %di
	movswl	%di, %eax
	addl	$10, %eax
	retq
.Lfunc_end0:
	.size	foo, .Lfunc_end0-foo
                                        # -- End function
	.section	".note.GNU-stack","",@progbits
