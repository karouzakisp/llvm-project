; RUN: opt -mtriple=riscv64 -mattr=+m,+a,+c -passes=wia -S < %s | FileCheck %s

; RV64 word arithmetic already supplies the signed i64 result, so removing
; this free extension does not justify promotion.
define i64 @load_add_sext(ptr %p) {
; CHECK-LABEL: define i64 @load_add_sext(
; CHECK:      %x = load i32, ptr %p
; CHECK-NEXT: %a = add nsw i32 %x, 1
; CHECK-NEXT: %e = sext i32 %a to i64
; CHECK-NEXT: ret i64 %e
entry:
  %x = load i32, ptr %p
  %a = add nsw i32 %x, 1
  %e = sext i32 %a to i64
  ret i64 %e
}

; The unsigned no-wrap addition can be promoted. Both forms lower to LWU
; followed by ADDI, so promotion does not change the instruction sequence.
define i64 @load_add_zext(ptr %p) {
; CHECK-LABEL: define i64 @load_add_zext(
; CHECK:      %x = load i32, ptr %p
; CHECK-NEXT: %0 = zext i32 %x to i64
; CHECK-NEXT: %a = add i64 %0, 1
; CHECK-NEXT: ret i64 %a
entry:
  %x = load i32, ptr %p
  %a = add nuw i32 %x, 1
  %e = zext i32 %a to i64
  ret i64 %e
}

define i32 @load_i8_add_zext(ptr %p) {
; CHECK-LABEL: define i32 @load_i8_add_zext(
; CHECK:      %x = load i8, ptr %p
; CHECK-NEXT: %0 = zext i8 %x to i32
; CHECK-NEXT: %a = add i32 %0, 1
; CHECK-NEXT: ret i32 %a
entry:
  %x = load i8, ptr %p
  %a = add nuw i8 %x, 1
  %e = zext i8 %a to i32
  ret i32 %e
}
