; RUN: opt -mtriple=riscv64 -mattr=+m,+a,+c -passes=wia -S < %s | FileCheck %s --check-prefix=CHECK

; CHECK-LABEL: define i6 @lshr_low_bits_only_narrow_graph
; CHECK-NOT: zext
; CHECK: %shr = lshr i8 %x, 2
; CHECK: trunc i8 %shr to i6
; CHECK: ret i6
define i6 @lshr_low_bits_only_narrow_graph(i8 %x) {
entry:
  %ext = zext i8 %x to i32
  %shr = lshr i32 %ext, 2
  %t = trunc i32 %shr to i6
  ret i6 %t
}

; CHECK-LABEL: define i8 @lshr_high_bits_not_known_zero
; CHECK: %ext = zext i8 %x to i32
; CHECK: %wide = or i32 %ext, 1024
; CHECK: %shr = lshr i32 %wide, 4
; CHECK: %t = trunc i32 %shr to i8
; CHECK: ret i8 %t
define i8 @lshr_high_bits_not_known_zero(i8 %x) {
entry:
  %ext = zext i8 %x to i32
  %wide = or i32 %ext, 1024
  %shr = lshr i32 %wide, 4
  %t = trunc i32 %shr to i8
  ret i8 %t
}

; CHECK-LABEL: define i8 @lshr_demands_extended_zero_bits
; CHECK: %shr = lshr i8 %x, 2
; CHECK: ret i8 %shr
define i8 @lshr_demands_extended_zero_bits(i8 %x) {
entry:
  %ext = zext i8 %x to i32
  %shr = lshr i32 %ext, 2
  %t = trunc i32 %shr to i8
  ret i8 %t
}
