	.text
	.file	"rv64_wia4.ll"
	.globl	test_load_and_cmp               # -- Begin function test_load_and_cmp
	.p2align	4, 0x90
	.type	test_load_and_cmp,@function
test_load_and_cmp:                      # @test_load_and_cmp
# %bb.0:
	movq	gd@GOTPCREL(%rip), %rax
	movzwl	(%rax), %ecx
	movq	gd1@GOTPCREL(%rip), %rax
	movzwl	(%rax), %eax
	movq	gd2@GOTPCREL(%rip), %rsi
	xorl	%edx, %edx
	divq	%rcx
	xorl	%edx, %edx
	divq	(%rsi)
	retq
.Lfunc_end0:
	.size	test_load_and_cmp, .Lfunc_end0-test_load_and_cmp
                                        # -- End function
	.section	".note.GNU-stack","",@progbits
