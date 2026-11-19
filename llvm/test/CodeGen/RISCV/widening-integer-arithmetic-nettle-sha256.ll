; RUN: opt -mtriple=riscv64 -mattr=+m,+a,+c -passes=wia,verify -S < %s | FileCheck %s

; A zero-filled PHI can require materializing extensions on incoming edges.
; Keep this singleton PHI narrow when promotion has no sufficient benefit.
; CHECK-LABEL: define i64 @phi_zext(
; CHECK: %p = phi i32
; CHECK-NEXT: %r = zext i32 %p to i64
; CHECK-NEXT: ret i64 %r

target triple = "riscv64"
define i64 @phi_zext(i32 %x) minsize {
entry:
 %inc = add i32 %x, 1
 %w = zext i32 %inc to i64
 %c = icmp ugt i64 %w, 56
 br i1 %c, label %zero, label %join
zero:
 br label %join
join:
 %p = phi i32 [ 0, %zero ], [ %inc, %entry ]
 %r = zext i32 %p to i64
 ret i64 %r
}
