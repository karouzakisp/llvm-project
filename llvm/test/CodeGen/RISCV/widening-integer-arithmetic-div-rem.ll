; RUN: opt -mtriple=riscv64 -mattr=+m,+a,+c -passes=wia,verify -S < %s | FileCheck %s

; CHECK-LABEL: define i8 @urem_safe_const_divisor
; CHECK: %r = urem i8 %x, 7
; CHECK: ret i8 %r
define i8 @urem_safe_const_divisor(i8 %x) {
entry:
  %ext = zext i8 %x to i32
  %r = urem i32 %ext, 7
  %t = trunc i32 %r to i8
  ret i8 %t
}

; CHECK-LABEL: define i8 @urem_divisor_truncates_to_zero
; CHECK: %ext = zext i8 %x to i32
; CHECK: %r = urem i32 %ext, 256
; CHECK: %t = trunc i32 %r to i8
; CHECK: ret i8 %t
define i8 @urem_divisor_truncates_to_zero(i8 %x) {
entry:
  %ext = zext i8 %x to i32
  %r = urem i32 %ext, 256
  %t = trunc i32 %r to i8
  ret i8 %t
}

; CHECK-LABEL: define i8 @urem_high_dividend_changes_result
; CHECK: %ext = zext i8 %x to i32
; CHECK: %wide = or i32 %ext, 256
; CHECK: %r = urem i32 %wide, 255
; CHECK: %t = trunc i32 %r to i8
; CHECK: ret i8 %t
define i8 @urem_high_dividend_changes_result(i8 %x) {
entry:
  %ext = zext i8 %x to i32
  %wide = or i32 %ext, 256
  %r = urem i32 %wide, 255
  %t = trunc i32 %r to i8
  ret i8 %t
}

; CHECK-LABEL: define i8 @udiv_safe_const_divisor
; CHECK: %q = udiv i8 %x, 7
; CHECK: ret i8 %q
define i8 @udiv_safe_const_divisor(i8 %x) {
entry:
  %ext = zext i8 %x to i32
  %q = udiv i32 %ext, 7
  %t = trunc i32 %q to i8
  ret i8 %t
}

; CHECK-LABEL: define i8 @udiv_divisor_truncates_to_zero
; CHECK: %ext = zext i8 %x to i32
; CHECK: %q = udiv i32 %ext, 256
; CHECK: %t = trunc i32 %q to i8
; CHECK: ret i8 %t
define i8 @udiv_divisor_truncates_to_zero(i8 %x) {
entry:
  %ext = zext i8 %x to i32
  %q = udiv i32 %ext, 256
  %t = trunc i32 %q to i8
  ret i8 %t
}

; CHECK-LABEL: define i8 @udiv_high_dividend_changes_result
; CHECK: %ext = zext i8 %x to i32
; CHECK: %wide = or i32 %ext, 256
; CHECK: %q = udiv i32 %wide, 255
; CHECK: %t = trunc i32 %q to i8
; CHECK: ret i8 %t
define i8 @udiv_high_dividend_changes_result(i8 %x) {
entry:
  %ext = zext i8 %x to i32
  %wide = or i32 %ext, 256
  %q = udiv i32 %wide, 255
  %t = trunc i32 %q to i8
  ret i8 %t
}

; CHECK-LABEL: define i8 @sdiv_safe_const_divisor
; CHECK: %q = sdiv i8 %x, 7
; CHECK: ret i8 %q
define i8 @sdiv_safe_const_divisor(i8 %x) {
entry:
  %ext = sext i8 %x to i32
  %q = sdiv i32 %ext, 7
  %t = trunc i32 %q to i8
  ret i8 %t
}

; CHECK-LABEL: define i8 @srem_safe_const_divisor
; CHECK: %r = srem i8 %x, 7
; CHECK: ret i8 %r
define i8 @srem_safe_const_divisor(i8 %x) {
entry:
  %ext = sext i8 %x to i32
  %r = srem i32 %ext, 7
  %t = trunc i32 %r to i8
  ret i8 %t
}

; CHECK-LABEL: define i8 @sdiv_minus_one_stays_wide
; CHECK: %ext = sext i8 %x to i32
; CHECK: %q = sdiv i32 %ext, -1
; CHECK: %t = trunc i32 %q to i8
; CHECK: ret i8 %t
define i8 @sdiv_minus_one_stays_wide(i8 %x) {
entry:
  %ext = sext i8 %x to i32
  %q = sdiv i32 %ext, -1
  %t = trunc i32 %q to i8
  ret i8 %t
}

; CHECK-LABEL: define i8 @sdiv_const_128_stays_wide
; CHECK: %ext = sext i8 %x to i32
; CHECK: %q = sdiv i32 %ext, 128
; CHECK: %t = trunc i32 %q to i8
; CHECK: ret i8 %t
define i8 @sdiv_const_128_stays_wide(i8 %x) {
entry:
  %ext = sext i8 %x to i32
  %q = sdiv i32 %ext, 128
  %t = trunc i32 %q to i8
  ret i8 %t
}

; CHECK-LABEL: define i8 @sdiv_ext_operand1_stays_wide
; CHECK: %ext = sext i8 %x to i32
; CHECK: %q = sdiv i32 42, %ext
; CHECK: %t = trunc i32 %q to i8
; CHECK: ret i8 %t
define i8 @sdiv_ext_operand1_stays_wide(i8 %x) {
entry:
  %ext = sext i8 %x to i32
  %q = sdiv i32 42, %ext
  %t = trunc i32 %q to i8
  ret i8 %t
}

; CHECK-LABEL: @sdiv_widens_division(
; CHECK: [[EXT:%.*]] = sext i8 %x to i64
; CHECK-NEXT: [[QUOT:%.*]] = sdiv i64 [[EXT]], 7
; CHECK-NEXT: ret i64 [[QUOT]]
define i64 @sdiv_widens_division(i8 %x) minsize {
entry:
  %q = sdiv i8 %x, 7
  %ext = sext i8 %q to i64
  ret i64 %ext
}
