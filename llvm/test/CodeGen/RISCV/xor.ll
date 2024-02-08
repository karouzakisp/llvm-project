;RUN: opt %s -mtriple=riscv64 | FileCheck %s

@gd = external global i32
@gd1 = external global i32
@gd2 = external global i64


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
  %ld = load i32, ptr @gd
  %ld1 = load i32, ptr @gd1
  %k = load i64, ptr @gd2

  %y = zext i32 %ld to i64
  %x = zext i32 %ld1 to i64

  %ud = xor i64 %x, %y

  %res = udiv i64 %ud, %k

  ret i64 %res;
}
