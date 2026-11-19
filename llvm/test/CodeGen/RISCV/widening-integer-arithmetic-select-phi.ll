; RUN: opt -mtriple=riscv64 -mattr=+m,+a,+c -passes=wia -S < %s | FileCheck %s

; CHECK-LABEL: define i8 @select_narrow_graph
; CHECK-NOT: zext
; CHECK: %a = add i8 %x, 44
; CHECK: %b = xor i8 %x, -86
; CHECK: %s = select i1 %c, i8 %a, i8 %b
; CHECK: ret i8 %s
define i8 @select_narrow_graph(i8 %x, i1 %c) {
entry:
  %ext = zext i8 %x to i32
  %a = add i32 %ext, 300
  %b = xor i32 %ext, 170
  %s = select i1 %c, i32 %a, i32 %b
  %t = trunc i32 %s to i8
  ret i8 %t
}

; CHECK-LABEL: define i8 @phi_narrow_graph
; CHECK-NOT: zext
; CHECK: %a = add i8 %x, 44
; CHECK: %b = xor i8 %x, -86
; CHECK: %p = phi i8 [ %a, %then ], [ %b, %else ]
; CHECK: ret i8 %p
define i8 @phi_narrow_graph(i8 %x, i1 %c) {
entry:
  %ext = zext i8 %x to i32
  br i1 %c, label %then, label %else

then:
  %a = add i32 %ext, 300
  br label %merge

else:
  %b = xor i32 %ext, 170
  br label %merge

merge:
  %p = phi i32 [ %a, %then ], [ %b, %else ]
  %t = trunc i32 %p to i8
  ret i8 %t
}

; CHECK-LABEL: define i8 @loop_phi_narrow_graph
; CHECK-NOT: zext
; CHECK: phi i8
; CHECK: add i8
; CHECK: ret i8
define i8 @loop_phi_narrow_graph(i8 %x, i1 %c) {
entry:
  %ext = zext i8 %x to i32
  br label %loop

loop:
  %p = phi i32 [ %ext, %entry ], [ %add, %loop ]
  %add = add i32 %p, 1
  br i1 %c, label %exit, label %loop

exit:
  %t = trunc i32 %p to i8
  ret i8 %t
}

; CHECK-LABEL: define i8 @phi_external_operand_blocks_narrow_graph
; CHECK: %ext = zext i8 %x to i32
; CHECK: %p = phi i32 [ %ext, %left ], [ %y, %right ]
; CHECK: %t = trunc i32 %p to i8
; CHECK: ret i8 %t
define i8 @phi_external_operand_blocks_narrow_graph(i8 %x, i32 %y, i1 %c) {
entry:
  %ext = zext i8 %x to i32
  br i1 %c, label %left, label %right

left:
  br label %merge

right:
  br label %merge

merge:
  %p = phi i32 [ %ext, %left ], [ %y, %right ]
  %t = trunc i32 %p to i8
  ret i8 %t
}
