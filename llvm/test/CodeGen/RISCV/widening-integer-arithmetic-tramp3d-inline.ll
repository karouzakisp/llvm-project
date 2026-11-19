; REQUIRES: riscv-registered-target
; RUN: opt -mtriple=riscv64 -mattr=+m,+a,+c,+f,+d,+zba,+zbb -disable-wia=true -passes='default<O2>' -verify-each -S %s | FileCheck %s
; RUN: opt -mtriple=riscv64 -mattr=+m,+a,+c,+f,+d,+zba,+zbb -disable-wia=false -passes='default<O2>' -verify-each -S %s | FileCheck %s
;
; Reduced from tramp3d-v4's FieldEngine::data and LeafFunctor::apply.
; With minsize, widening the helper's index arithmetic can increase its inline
; cost enough to keep the call. That hides its read-only body from the caller
; and prevents the otherwise unobservable nested loops from being deleted.
; Both runs should retain the same optimization: dead_loop returns zero with
; no remaining calls or loops.

target datalayout = "e-m:e-p:64:64-i64:64-i128:128-n32:64-S128"

$get_data = comdat any
$dead_loop = comdat any

define weak_odr dso_local noundef nonnull align 8 dereferenceable(88) ptr @get_data(ptr noundef nonnull align 8 dereferenceable(144) %engine, i32 noundef signext %material, i32 noundef signext %centering) #0 comdat align 2 {
entry:
  %offset.addr = getelementptr inbounds nuw i8, ptr %engine, i64 72
  %stride.addr = getelementptr inbounds nuw i8, ptr %engine, i64 64
  %stride = load i32, ptr %stride.addr, align 8
  %mul = mul nsw i32 %material, %stride
  %add = add nsw i32 %mul, %centering
  %index = sext i32 %add to i64
  %controller.addr = getelementptr inbounds nuw i8, ptr %engine, i64 80
  %controller = load ptr, ptr %controller.addr, align 8
  %begin.addr = getelementptr inbounds nuw i8, ptr %controller, i64 8
  %begin = load ptr, ptr %begin.addr, align 8
  %offset = load i64, ptr %offset.addr, align 8
  %base = getelementptr inbounds [88 x i8], ptr %begin, i64 %offset
  %result = getelementptr inbounds [88 x i8], ptr %base, i64 %index
  ret ptr %result
}

; CHECK-LABEL: define {{.*}}i32 @dead_loop(
; CHECK-NEXT: entry:
; CHECK-NEXT: ret i32 0
; CHECK-NEXT: }
define weak_odr dso_local noundef signext i32 @dead_loop(ptr noundef nonnull align 8 dereferenceable(144) %engine, ptr noundef nonnull align 8 dereferenceable(8) %tag) #0 comdat align 2 {
entry:
  br label %outer

outer:
  %material = phi i32 [ 0, %entry ], [ %material.next, %outer.latch ]
  %materials = load i32, ptr %engine, align 8
  %outer.more = icmp slt i32 %material, %materials
  br i1 %outer.more, label %inner, label %exit

inner:
  %centering = phi i32 [ 0, %outer ], [ %centering.next, %body ]
  %start.addr = getelementptr inbounds nuw i8, ptr %engine, i64 16
  %finish.addr = getelementptr inbounds nuw i8, ptr %engine, i64 24
  %finish = load ptr, ptr %finish.addr, align 8
  %start = load ptr, ptr %start.addr, align 8
  %finish.int = ptrtoaddr ptr %finish to i64
  %start.int = ptrtoaddr ptr %start to i64
  %bytes = sub i64 %finish.int, %start.int
  %elements = sdiv exact i64 %bytes, 12
  %centerings = trunc i64 %elements to i32
  %inner.more = icmp slt i32 %centering, %centerings
  br i1 %inner.more, label %body, label %outer.latch

body:
  %unused = call noundef nonnull align 8 dereferenceable(88) ptr @get_data(ptr noundef nonnull align 8 dereferenceable(144) %engine, i32 noundef signext %material, i32 noundef signext %centering) #1
  %centering.next = add nuw nsw i32 %centering, 1
  br label %inner

outer.latch:
  %material.next = add nuw nsw i32 %material, 1
  br label %outer

exit:
  ret i32 0
}

attributes #0 = { inlinehint minsize mustprogress nounwind optsize }
attributes #1 = { minsize optsize }
