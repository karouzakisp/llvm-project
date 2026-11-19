; RUN: opt -mtriple=riscv64 -mattr=+m,+a,+c -passes=wia -S < %s | FileCheck %s --check-prefix=CHECK

define i32 @promote_source_sext_const_add() {
; CHECK-LABEL: define i32 @promote_source_sext_const_add
; CHECK: %sum = add nsw i32 -5, -7
; CHECK-NOT: sext i8 %sum to i32
entry:
  %sum = add nsw i8 -5, -7
  %ext = sext i8 %sum to i32
  ret i32 %ext
}

define i32 @promote_source_select_const(i1 %c) {
; CHECK-LABEL: define i32 @promote_source_select_const
; CHECK: %s = select i1 %c, i32 5, i32 7
; CHECK: %sum = add i32 %s, 1
; CHECK-NOT: zext i8 %sum to i32
entry:
  %s = select i1 %c, i8 5, i8 7
  %sum = add nuw i8 %s, 1
  %ext = zext i8 %sum to i32
  ret i32 %ext
}

define i32 @promote_source_shared_producer(i1 %c) {
; CHECK-LABEL: define i32 @promote_source_shared_producer
; CHECK: %base = add i32 4, 3
; CHECK: %a = add i32 %base, 1
; CHECK: %b = xor i32 %base, 15
; CHECK: %s = select i1 %c, i32 %a, i32 %b
; CHECK-NOT: zext i8 %s to i32
entry:
  %base = add nuw i8 4, 3
  %a = add nuw i8 %base, 1
  %b = xor i8 %base, 15
  %s = select i1 %c, i8 %a, i8 %b
  %ext = zext i8 %s to i32
  ret i32 %ext
}

; The shared i8 -1 needs SIGN fill for the addition and ZEROS fill for lshr.
; Widen each use separately; 1 + (-1) is defined with nsw.
define i32 @promote_source_same_constant_two_uses() {
; CHECK-LABEL: define i32 @promote_source_same_constant_two_uses
; CHECK: %a = add nsw i32 1, -1
; CHECK: %b = lshr i32 255, 4
; CHECK: %sum = add i32 %a, %b
; CHECK-NOT: zext i8 %sum to i32
entry:
  %a = add nsw i8 1, -1
  %b = lshr i8 -1, 4
  %sum = add nuw i8 %a, %b
  %ext = zext i8 %sum to i32
  ret i32 %ext
}
