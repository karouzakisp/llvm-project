	.text
	.file	"xor.ll"
	.globl	test_load_and_cmp               # -- Begin function test_load_and_cmp
	.p2align	4, 0x90
	.type	test_load_and_cmp,@function
test_load_and_cmp:                      # @test_load_and_cmp
# %bb.0:
	movq	gd@GOTPCREL(%rip), %rcx
	movq	gd1@GOTPCREL(%rip), %rax
	movl	(%rax), %eax
	movq	gd2@GOTPCREL(%rip), %rsi
	xorl	(%rcx), %eax
	xorl	%edx, %edx
	divq	(%rsi)
	retq
.Lfunc_end0:
	.size	test_load_and_cmp, .Lfunc_end0-test_load_and_cmp
                                        # -- End function
	.section	".note.GNU-stack","",@progbits
