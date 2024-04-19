; ModuleID = 'mytest.cpp'
source_filename = "mytest.cpp"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

@.str = private unnamed_addr constant [11 x i8] c"Sum is %d\0A\00", align 1

; Function Attrs: mustprogress nofree norecurse nounwind uwtable
define dso_local noundef i32 @main() local_unnamed_addr #0 {
entry:
  br label %for.body

for.body:                                         ; preds = %entry, %for.inc34
  %indvars.iv = phi i32 [ 1, %entry ], [ %indvars.iv.next, %for.inc34 ]
  %sum.060 = phi i32 [ 0, %entry ], [ %sum.4, %for.inc34 ]
  switch i32 %indvars.iv, label %vector.ph [
    i32 101, label %for.inc34
    i32 1005, label %for.inc34
  ]

vector.ph:                                        ; preds = %for.body, %middle.block
  %sum.156 = phi i32 [ %15, %middle.block ], [ %sum.060, %for.body ]
  %j.155 = phi i16 [ %inc32, %middle.block ], [ 0, %for.body ]
  %0 = zext i16 %j.155 to i32
  %tobool26 = icmp ne i32 %indvars.iv, %0
  %conv28.neg = sext i1 %tobool26 to i32
  %1 = insertelement <4 x i32> <i32 poison, i32 0, i32 0, i32 0>, i32 %sum.156, i64 0
  %broadcast.splatinsert = insertelement <4 x i32> poison, i32 %conv28.neg, i64 0
  %broadcast.splat = shufflevector <4 x i32> %broadcast.splatinsert, <4 x i32> poison, <4 x i32> zeroinitializer
  %broadcast.splatinsert67 = insertelement <4 x i32> poison, i32 %conv28.neg, i64 0
  %broadcast.splat68 = shufflevector <4 x i32> %broadcast.splatinsert67, <4 x i32> poison, <4 x i32> zeroinitializer
  br label %vector.body

vector.body:                                      ; preds = %vector.body, %vector.ph
  %index = phi i32 [ 0, %vector.ph ], [ %index.next, %vector.body ]
  %vec.phi = phi <4 x i32> [ %1, %vector.ph ], [ %12, %vector.body ]
  %vec.phi65 = phi <4 x i32> [ zeroinitializer, %vector.ph ], [ %13, %vector.body ]
  %vec.ind = phi <4 x i16> [ <i16 0, i16 1, i16 2, i16 3>, %vector.ph ], [ %vec.ind.next, %vector.body ]
  %2 = and <4 x i16> %vec.ind, <i16 1, i16 1, i16 1, i16 1>
  %3 = and <4 x i16> %vec.ind, <i16 1, i16 1, i16 1, i16 1>
  %4 = icmp eq <4 x i16> %2, zeroinitializer
  %5 = icmp eq <4 x i16> %3, zeroinitializer
  %6 = icmp ne <4 x i16> %vec.ind, zeroinitializer
  %7 = icmp ne <4 x i16> %vec.ind, <i16 -4, i16 -4, i16 -4, i16 -4>
  %8 = zext <4 x i1> %6 to <4 x i32>
  %9 = zext <4 x i1> %7 to <4 x i32>
  %10 = select <4 x i1> %4, <4 x i32> %8, <4 x i32> %broadcast.splat
  %11 = select <4 x i1> %5, <4 x i32> %9, <4 x i32> %broadcast.splat68
  %12 = add <4 x i32> %10, %vec.phi
  %13 = add <4 x i32> %11, %vec.phi65
  %index.next = add nuw i32 %index, 8
  %vec.ind.next = add <4 x i16> %vec.ind, <i16 8, i16 8, i16 8, i16 8>
  %14 = icmp eq i32 %index.next, 1000
  br i1 %14, label %middle.block, label %vector.body, !llvm.loop !5

middle.block:                                     ; preds = %vector.body
  %bin.rdx = add <4 x i32> %13, %12
  %15 = tail call i32 @llvm.vector.reduce.add.v4i32(<4 x i32> %bin.rdx)
  %inc32 = add nuw nsw i16 %j.155, 1
  %exitcond62.not = icmp eq i16 %inc32, 100
  br i1 %exitcond62.not, label %for.inc34, label %vector.ph, !llvm.loop !9

for.inc34:                                        ; preds = %middle.block, %for.body, %for.body
  %sum.4 = phi i32 [ %sum.060, %for.body ], [ %sum.060, %for.body ], [ %15, %middle.block ]
  %indvars.iv.next = add nuw nsw i32 %indvars.iv, 1
  %exitcond64.not = icmp eq i32 %indvars.iv.next, 50
  br i1 %exitcond64.not, label %for.end36, label %for.body, !llvm.loop !10

for.end36:                                        ; preds = %for.inc34
  %call = tail call i32 (ptr, ...) @printf(ptr noundef nonnull dereferenceable(1) @.str, i32 noundef %sum.4)
  ret i32 0
}

; Function Attrs: nofree nounwind
declare noundef i32 @printf(ptr nocapture noundef readonly, ...) local_unnamed_addr #1

; Function Attrs: nocallback nofree nosync nounwind willreturn memory(none)
declare i32 @llvm.vector.reduce.add.v4i32(<4 x i32>) #2

attributes #0 = { mustprogress nofree norecurse nounwind uwtable "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #1 = { nofree nounwind "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #2 = { nocallback nofree nosync nounwind willreturn memory(none) }

!llvm.module.flags = !{!0, !1, !2, !3}
!llvm.ident = !{!4}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 8, !"PIC Level", i32 2}
!2 = !{i32 7, !"PIE Level", i32 2}
!3 = !{i32 7, !"uwtable", i32 2}
!4 = !{!"clang version 16.0.6 (git@github.com:karouzakisp/llvm-project.git 96976f687df5a5c8040ba691c8efe3f8c3368032)"}
!5 = distinct !{!5, !6, !7, !8}
!6 = !{!"llvm.loop.mustprogress"}
!7 = !{!"llvm.loop.isvectorized", i32 1}
!8 = !{!"llvm.loop.unroll.runtime.disable"}
!9 = distinct !{!9, !6}
!10 = distinct !{!10, !6}
