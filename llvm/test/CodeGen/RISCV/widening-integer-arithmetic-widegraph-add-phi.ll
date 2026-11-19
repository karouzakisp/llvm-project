; RUN: opt -mtriple=riscv64 -mattr=+m,+a,+c -passes=wia -S < %s | FileCheck %s --check-prefix=CHECK

define i32 @promote_source_const_add() {
; CHECK-LABEL: define i32 @promote_source_const_add
; CHECK: %sum = add i32 5, 7
; CHECK-NOT: zext i8 %sum to i32
entry:
  %sum = add nuw i8 5, 7
  %ext = zext i8 %sum to i32
  ret i32 %ext
}

define i32 @no_promote_external_args(i8 %a, i8 %b) {
; CHECK-LABEL: define i32 @no_promote_external_args
; CHECK: %sum = add nuw i8 %a, %b
; CHECK: %ext = zext i8 %sum to i32
entry:
  %sum = add nuw i8 %a, %b
  %ext = zext i8 %sum to i32
  ret i32 %ext
}

define i32 @promote_source_const_sub() {
; CHECK-LABEL: define i32 @promote_source_const_sub
; CHECK: %sub = sub i32 20, 7
; CHECK-NOT: zext i8 %sub to i32
entry:
  %sub = sub nuw i8 20, 7
  %ext = zext i8 %sub to i32
  ret i32 %ext
}
