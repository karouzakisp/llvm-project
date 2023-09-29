	.text
	.attribute	4, 16
	.attribute	5, "rv64i2p0"
	.file	"rv64_wia1.ll"
	.globl	test_load_and_cmp               # -- Begin function test_load_and_cmp
	.p2align	2
	.type	test_load_and_cmp,@function
test_load_and_cmp:                      # @test_load_and_cmp
# %bb.0:
	addw	a0, a0, a1
	ret
.Lfunc_end0:
	.size	test_load_and_cmp, .Lfunc_end0-test_load_and_cmp
                                        # -- End function
	.section	".note.GNU-stack","",@progbits
