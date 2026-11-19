; RUN: opt -mtriple=riscv64 -mattr=+m,+a,+c -passes=wia,verify -S < %s | FileCheck %s

target triple = "riscv64"
; Proving signed no-wrap does not prove a zero-filled result. For x=0,
; the original returns 4294967277; widening the add without repair returns -19.
; CHECK-LABEL: define i64 @signed_proof_zero_demand(
; CHECK:       [[A:%.*]] = zext i8 %x to i32
; CHECK-NEXT:  [[B:%.*]] = add i32 [[A]], -19
; CHECK-NEXT:  [[R:%.*]] = zext i32 [[B]] to i64
; CHECK-NEXT:  ret i64 [[R]]
define i64 @signed_proof_zero_demand(i8 %x) minsize {
  %a = zext i8 %x to i32
  %b = add i32 %a, -19
  %r = zext i32 %b to i64
  ret i64 %r
}
; Proving unsigned no-wrap does not prove a sign-filled result.
; CHECK-LABEL: define i64 @unsigned_proof_sign_demand(
; CHECK:       [[A:%.*]] = zext i8 %x to i16
; CHECK-NEXT:  [[B:%.*]] = add i16 [[A]], 32700
; CHECK-NEXT:  [[R:%.*]] = sext i16 [[B]] to i64
; CHECK-NEXT:  ret i64 [[R]]
define i64 @unsigned_proof_sign_demand(i8 %x) minsize {
  %a = zext i8 %x to i16
  %b = add i16 %a, 32700
  %r = sext i16 %b to i64
  ret i64 %r
}
; A compatible zero-fill proof must still allow promotion.
; CHECK-LABEL: define i64 @positive_zero_control(
; CHECK:       [[A:%.*]] = zext i8 %x to i64
; CHECK-NEXT:  [[B:%.*]] = add nsw i64 [[A]], 19
; CHECK-NEXT:  ret i64 [[B]]
define i64 @positive_zero_control(i8 %x) minsize {
  %a = zext i8 %x to i32
  %b = add i32 %a, 19
  %r = zext i32 %b to i64
  ret i64 %r
}
; A compatible sign-fill proof must still allow promotion.
; CHECK-LABEL: define i64 @positive_sign_control(
; CHECK:       [[A:%.*]] = zext i8 %x to i64
; CHECK-NEXT:  [[B:%.*]] = add nsw i64 [[A]], -19
; CHECK-NEXT:  ret i64 [[B]]
define i64 @positive_sign_control(i8 %x) minsize {
  %a = zext i8 %x to i16
  %b = add i16 %a, -19
  %r = sext i16 %b to i64
  ret i64 %r
}
