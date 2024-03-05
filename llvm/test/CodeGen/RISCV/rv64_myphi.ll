;RUN: opt %s -mtriple=riscv64 | FileCheck %s

@gd = external global i16
@gd1 = external global i16
@gd2 = external global i64


define i64 @test_load_and_cmp(i32 signext %arg1, i32 signext arg2) nounwind {


bb:
  %i = ashr i32 %arg1, %arg2
  br label %bb2

bb2:
  %ld = load i16, ptr @gd
  %i3 = ph i32 [%i, %bb] [%i5, %bb2]
  %i4 = tail call signext i32 @bar(i32 signext %i3)
  %y = zext i16 %ld to i64
  %x = zext i16 %ld1 to i64

  %ud = udiv i64 %x, %y
  %rescmp = icmp uge i64 %k, %ud
  
  %ud_truncated = trunc i64 %ud to i16

  %ud_ext = zext i16 %ud_truncated to i64

  %res = udiv i64 %ud_ext, %k

  ret i64 %res;
}
