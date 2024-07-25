; ModuleID = 'mytest.cpp'
source_filename = "mytest.cpp"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

@.str = private unnamed_addr constant [11 x i8] c"Sum is %d\0A\00", align 1

; Function Attrs: mustprogress nofree norecurse nounwind uwtable
define dso_local noundef i32 @main() local_unnamed_addr #0 {
entry:
  br label %for.body

for.body:                                         ; preds = %entry, %for.inc39
  %indvars.iv75 = phi i32 [ 1, %entry ], [ %indvars.iv.next76, %for.inc39 ]
  %sum.068 = phi i32 [ 0, %entry ], [ %sum.4, %for.inc39 ]
  switch i32 %indvars.iv75, label %vector.ph [
    i32 101, label %for.inc39
    i32 1005, label %for.inc39
  ]

vector.ph:                                        ; preds = %for.body, %middle.block
  %indvars.iv71 = phi i32 [ %indvars.iv.next72, %middle.block ], [ 0, %for.body ]
  %sum.163 = phi i32 [ %24, %middle.block ], [ %sum.068, %for.body ]
  %tobool31 = icmp ne i32 %indvars.iv75, %indvars.iv71
  %0 = insertelement <4 x i32> <i32 poison, i32 0, i32 0, i32 0>, i32 %sum.163, i64 0
  %broadcast.splatinsert = insertelement <4 x i32> poison, i32 %indvars.iv71, i64 0
  %broadcast.splat = shufflevector <4 x i32> %broadcast.splatinsert, <4 x i32> poison, <4 x i32> zeroinitializer
  %broadcast.splatinsert81 = insertelement <4 x i1> poison, i1 %tobool31, i64 0
  %broadcast.splat82 = shufflevector <4 x i1> %broadcast.splatinsert81, <4 x i1> poison, <4 x i32> zeroinitializer
  br label %vector.body

vector.body:                                      ; preds = %vector.body, %vector.ph
  %index = phi i32 [ 0, %vector.ph ], [ %index.next, %vector.body ]
  %vec.ind = phi <4 x i32> [ <i32 0, i32 1, i32 2, i32 3>, %vector.ph ], [ %vec.ind.next, %vector.body ]
  %vec.phi = phi <4 x i32> [ %0, %vector.ph ], [ %21, %vector.body ]
  %vec.phi80 = phi <4 x i32> [ zeroinitializer, %vector.ph ], [ %22, %vector.body ]
  %step.add = add <4 x i32> %vec.ind, <i32 4, i32 4, i32 4, i32 4>
  %1 = trunc <4 x i32> %vec.ind to <4 x i1>
  %2 = trunc <4 x i32> %vec.ind to <4 x i1>
  %3 = sub nsw <4 x i32> %broadcast.splat, %vec.ind
  %4 = sub nsw <4 x i32> %broadcast.splat, %step.add
  %5 = and <4 x i32> %3, <i32 -2147483645, i32 -2147483645, i32 -2147483645, i32 -2147483645>
  %6 = and <4 x i32> %4, <i32 -2147483645, i32 -2147483645, i32 -2147483645, i32 -2147483645>
  %7 = icmp eq <4 x i32> %5, <i32 3, i32 3, i32 3, i32 3>
  %8 = icmp eq <4 x i32> %6, <i32 3, i32 3, i32 3, i32 3>
  %9 = icmp ne <4 x i32> %vec.ind, zeroinitializer
  %10 = icmp ne <4 x i32> %step.add, zeroinitializer
  %11 = zext <4 x i1> %9 to <4 x i32>
  %12 = zext <4 x i1> %10 to <4 x i32>
  %13 = select <4 x i1> %1, <4 x i1> <i1 true, i1 true, i1 true, i1 true>, <4 x i1> %7
  %14 = select <4 x i1> %2, <4 x i1> <i1 true, i1 true, i1 true, i1 true>, <4 x i1> %8
  %15 = icmp ne <4 x i32> %vec.ind, zeroinitializer
  %16 = icmp ne <4 x i32> %step.add, zeroinitializer
  %17 = and <4 x i1> %broadcast.splat82, %15
  %18 = and <4 x i1> %broadcast.splat82, %16
  %19 = sext <4 x i1> %17 to <4 x i32>
  %20 = sext <4 x i1> %18 to <4 x i32>
  %predphi = select <4 x i1> %13, <4 x i32> %19, <4 x i32> %11
  %predphi83 = select <4 x i1> %14, <4 x i32> %20, <4 x i32> %12
  %21 = add <4 x i32> %predphi, %vec.phi
  %22 = add <4 x i32> %predphi83, %vec.phi80
  %index.next = add nuw i32 %index, 8
  %vec.ind.next = add <4 x i32> %vec.ind, <i32 8, i32 8, i32 8, i32 8>
  %23 = icmp eq i32 %index.next, 1000
  br i1 %23, label %middle.block, label %vector.body, !llvm.loop !5

middle.block:                                     ; preds = %vector.body
  %bin.rdx = add <4 x i32> %22, %21
  %24 = tail call i32 @llvm.vector.reduce.add.v4i32(<4 x i32> %bin.rdx)
  %indvars.iv.next72 = add nuw nsw i32 %indvars.iv71, 1
  %exitcond74.not = icmp eq i32 %indvars.iv.next72, 100
  br i1 %exitcond74.not, label %for.inc39, label %vector.ph, !llvm.loop !9

for.inc39:                                        ; preds = %middle.block, %for.body, %for.body
  %sum.4 = phi i32 [ %sum.068, %for.body ], [ %sum.068, %for.body ], [ %24, %middle.block ]
  %indvars.iv.next76 = add nuw nsw i32 %indvars.iv75, 1
  %exitcond78.not = icmp eq i32 %indvars.iv.next76, 50
  br i1 %exitcond78.not, label %for.end41, label %for.body, !llvm.loop !10

for.end41:                                        ; preds = %for.inc39
  %call = tail call i32 (ptr, ...) @printf(ptr noundef nonnull dereferenceable(1) @.str, i32 noundef %sum.4)
  ret i32 0
}

; Function Attrs: nofree nounwind
declare noundef i32 @printf(ptr nocapture noundef readonly, ...) local_unnamed_addr #1

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare i32 @llvm.vector.reduce.add.v4i32(<4 x i32>) #2

attributes #0 = { mustprogress nofree norecurse nounwind uwtable "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #1 = { nofree nounwind "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #2 = { nocallback nofree nosync nounwind speculatable willreturn memory(none) }

!llvm.module.flags = !{!0, !1, !2, !3}
!llvm.ident = !{!4}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 8, !"PIC Level", i32 2}
!2 = !{i32 7, !"PIE Level", i32 2}
!3 = !{i32 7, !"uwtable", i32 2}
!4 = !{!"clang version 19.0.0git (git@github.com:karouzakisp/llvm-project.git 3d78e495e619c4f42443d5464ac48da5dd3959a7)"}
!5 = distinct !{!5, !6, !7, !8}
!6 = !{!"llvm.loop.mustprogress"}
!7 = !{!"llvm.loop.isvectorized", i32 1}
!8 = !{!"llvm.loop.unroll.runtime.disable"}
!9 = distinct !{!9, !6}
!10 = distinct !{!10, !6}
