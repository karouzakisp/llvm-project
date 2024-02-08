;RUN: opt %s -mtriple=riscv64 | FileCheck %s

@gd = external global i16
@gf = external global i32

define i32 @test_load_and_cmp(i16 %a) nounwind {
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
	%ff_trunc = trunc i32 %ff to i16
  %q = xor i16 %ff_trunc, %k
  %sexted_q = sext i16 %q to i32
  %res = add i32 %sexted_q, 10
  ret i32 %res;
}

