;RUN: opt %s -mtriple=riscv64 | FileCheck %s

@gd = external global i16
@gf = external global i32

define i64 @test_load_and_cmp(i16 %a) nounwind {
; RV64I-LABEL: test_load_and_cmp:
; RV64I:       # %bb.0:
; RV64I-NEXT:    lui a1, %hi(gd)
; RV64I-NEXT:    lh a1, %lo(gd)(a1)
; RV64I-NEXT:    xor a0, a0, a1
; RV64I-NEXT:    slli a0, a0, 48
; RV64I-NEXT:    srai a0, a0, 48
; RV64I-NEXT:    addi a0, a0, 10
; RV64I-NEXT:    ret
  %k = load i16, ptr @gd
	%ff = load i32, ptr @gf
  %sexted_q = zext i32 %ff to i64
  %sexted_k = zext i16 %k to i64
  %q = add i64 %sexted_q, %sexted_k
  ret i64 %q;
}

