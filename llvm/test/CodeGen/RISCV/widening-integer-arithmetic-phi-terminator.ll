; RUN: opt -mtriple=riscv64 -passes=wia,verify -S < %s | FileCheck %s

; Multiple extension users make PHI promotion profitable. Incoming extensions
; must nevertheless have a legal insertion point and an available operand.

define i64 @callbr_incoming(ptr %out1, ptr %out2) minsize {
; CHECK-LABEL: define i64 @callbr_incoming(
; CHECK:       entry:
; CHECK-NEXT:    %v = callbr i16 asm "", "=r,!i"()
; CHECK-NEXT:            to label %normal [label %other]
; CHECK:       normal:
; CHECK-NEXT:    %p = phi i16 [ %v, %entry ], [ 7, %other ]
; CHECK-NEXT:    %e1 = sext i16 %p to i64
entry:
  %v = callbr i16 asm "", "=r,!i"() to label %normal [label %other]
normal:
  %p = phi i16 [ %v, %entry ], [ 7, %other ]
  %e1 = sext i16 %p to i64
  store i64 %e1, ptr %out1
  %e2 = sext i16 %p to i64
  store i64 %e2, ptr %out2
  %e3 = sext i16 %p to i64
  ret i64 %e3
other:
  br label %normal
}

declare i16 @produce()
declare i32 @__gxx_personality_v0(...)

define i64 @invoke_incoming(ptr %out1, ptr %out2) minsize personality ptr @__gxx_personality_v0 {
; CHECK-LABEL: define i64 @invoke_incoming(
; CHECK:       entry:
; CHECK-NEXT:    %v = invoke i16 @produce()
; CHECK-NEXT:            to label %normal unwind label %other
; CHECK:       normal:
; CHECK-NEXT:    %p = phi i16 [ %v, %entry ], [ 7, %other ]
; CHECK-NEXT:    %e1 = sext i16 %p to i64
entry:
  %v = invoke i16 @produce() to label %normal unwind label %other
normal:
  %p = phi i16 [ %v, %entry ], [ 7, %other ]
  %e1 = sext i16 %p to i64
  store i64 %e1, ptr %out1
  %e2 = sext i16 %p to i64
  store i64 %e2, ptr %out2
  %e3 = sext i16 %p to i64
  ret i64 %e3
other:
  %lp = landingpad { ptr, i32 } cleanup
  br label %normal
}

declare i32 @__CxxFrameHandler3(...)
declare void @maythrow()

define void @catchswitch_incoming(i16 %v, ptr %out1, ptr %out2, ptr %out3) minsize personality ptr @__CxxFrameHandler3 {
; CHECK-LABEL: define void @catchswitch_incoming(
; CHECK:       dispatch:
; CHECK-NEXT:    %cs = catchswitch within none [label %catch] unwind to caller
; CHECK:       catch:
; CHECK-NEXT:    %p = phi i16 [ %v, %dispatch ]
; CHECK-NEXT:    %cp = catchpad within %cs [ptr null, i32 64, ptr null]
; CHECK-NEXT:    %e1 = sext i16 %p to i64
entry:
  invoke void @maythrow() to label %exit unwind label %dispatch
dispatch:
  %cs = catchswitch within none [label %catch] unwind to caller
catch:
  %p = phi i16 [ %v, %dispatch ]
  %cp = catchpad within %cs [ptr null, i32 64, ptr null]
  %e1 = sext i16 %p to i64
  store i64 %e1, ptr %out1
  %e2 = sext i16 %p to i64
  store i64 %e2, ptr %out2
  %e3 = sext i16 %p to i64
  store i64 %e3, ptr %out3
  catchret from %cp to label %exit
exit:
  ret void
}

; An ordinary call result is available before its block's terminator, so its
; PHI can still be promoted by inserting the extension before the branch.
define i64 @call_incoming(i1 %c, ptr %out1, ptr %out2) minsize {
; CHECK-LABEL: define i64 @call_incoming(
; CHECK:       entry:
; CHECK-NEXT:    %v = call i16 @produce()
; CHECK-NEXT:    [[EXT:%.*]] = sext i16 %v to i64
; CHECK-NEXT:    br i1 %c, label %normal, label %other
; CHECK:       normal:
; CHECK-NEXT:    [[PHI:%.*]] = phi i64 [ [[EXT]], %entry ], [ 7, %other ]
; CHECK-NEXT:    store i64 [[PHI]], ptr %out1
; CHECK-NEXT:    store i64 [[PHI]], ptr %out2
; CHECK-NEXT:    ret i64 [[PHI]]
entry:
  %v = call i16 @produce()
  br i1 %c, label %normal, label %other
normal:
  %p = phi i16 [ %v, %entry ], [ 7, %other ]
  %e1 = sext i16 %p to i64
  store i64 %e1, ptr %out1
  %e2 = sext i16 %p to i64
  store i64 %e2, ptr %out2
  %e3 = sext i16 %p to i64
  ret i64 %e3
other:
  br label %normal
}
