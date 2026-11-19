; RUN: opt -mtriple=riscv64 -mattr=+m,+a,+c -passes=wia,verify -S < %s | FileCheck %s

; Promote this loop-carried i8 recurrence on RV64. This is the kind of cyclic
; narrow recurrence DAGCombiner cannot recover globally once SelectionDAG is
; built.
define i32 @loop_i8_load_nuw(ptr %p, i32 %n) {
; CHECK-LABEL: define i32 @loop_i8_load_nuw(
; CHECK: entry:
; CHECK:   [[INIT:%.*]] = load i8, ptr %p, align 1
; CHECK:   [[INIT_WIDE:%.*]] = zext i8 [[INIT]] to i32
; CHECK: loop:
; CHECK:   [[IV:%.*]] = phi i32 [ [[INIT_WIDE]], %entry ], [ [[NEXT:%.*]], %loop ]
; CHECK:   %acc.next = add i32 %acc, [[IV]]
; CHECK:   [[NEXT]] = add i32 [[IV]], 1
; CHECK:   [[CMP_IV:%.*]] = icmp ult i32 [[NEXT]], 100
; CHECK-NOT: zext i8 [[IV]] to i32
entry:
  %init = load i8, ptr %p, align 1
  br label %loop

loop:
  %iv = phi i8 [ %init, %entry ], [ %next, %loop ]
  %i = phi i32 [ 0, %entry ], [ %i.next, %loop ]
  %acc = phi i32 [ 0, %entry ], [ %acc.next, %loop ]
  %wide = zext i8 %iv to i32
  %acc.next = add i32 %acc, %wide
  %next = add nuw i8 %iv, 1
  %cmp.iv = icmp ult i8 %next, 100
  %i.next = add nuw i32 %i, 1
  %cmp.i = icmp ult i32 %i.next, %n
  %cont = and i1 %cmp.iv, %cmp.i
  br i1 %cont, label %loop, label %exit

exit:
  ret i32 %acc.next
}

; An argument initial value is extended once in the entry block, while the
; recurrence and comparison are promoted together.
define i32 @loop_i8_arg_nuw(i8 zeroext %init, i32 %n) {
; CHECK-LABEL: define i32 @loop_i8_arg_nuw(
; CHECK: entry:
; CHECK:   [[ARG_WIDE:%.*]] = zext i8 %init to i32
; CHECK: loop:
; CHECK:   [[IV:%.*]] = phi i32 [ [[ARG_WIDE]], %entry ], [ [[NEXT:%.*]], %loop ]
; CHECK:   %acc.next = add i32 %acc, [[IV]]
; CHECK:   [[NEXT]] = add i32 [[IV]], 1
; CHECK:   [[CMP_IV:%.*]] = icmp ult i32 [[NEXT]], 100
; CHECK-NOT: zext i8 [[IV]] to i32
entry:
  br label %loop

loop:
  %iv = phi i8 [ %init, %entry ], [ %next, %loop ]
  %i = phi i32 [ 0, %entry ], [ %i.next, %loop ]
  %acc = phi i32 [ 0, %entry ], [ %acc.next, %loop ]
  %wide = zext i8 %iv to i32
  %acc.next = add i32 %acc, %wide
  %next = add nuw i8 %iv, 1
  %cmp.iv = icmp ult i8 %next, 100
  %i.next = add nuw i32 %i, 1
  %cmp.i = icmp ult i32 %i.next, %n
  %cont = and i1 %cmp.iv, %cmp.i
  br i1 %cont, label %loop, label %exit

exit:
  ret i32 %acc.next
}
