; ModuleID = 'mytest.cpp'
source_filename = "mytest.cpp"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

@.str = private unnamed_addr constant [11 x i8] c"Sum is %d\0A\00", align 1

; Function Attrs: mustprogress nofree norecurse nounwind uwtable
define dso_local noundef i32 @main() local_unnamed_addr #0 {
entry:
  br label %for.body

for.body:                                         ; preds = %entry, %for.inc23
  %sum.044 = phi i32 [ 0, %entry ], [ %sum.4, %for.inc23 ]
  %k.043 = phi i16 [ 1, %entry ], [ %inc24, %for.inc23 ]
  %rem36.urem = and i16 %k.043, 1
  %cmp4.not = icmp eq i16 %rem36.urem, 0
  br i1 %cmp4.not, label %vector.ph, label %for.inc23

vector.ph:                                        ; preds = %for.body, %middle.block
  %sum.141 = phi i32 [ %12, %middle.block ], [ %sum.044, %for.body ]
  %j.140 = phi i16 [ %inc21, %middle.block ], [ 0, %for.body ]
  %0 = insertelement <4 x i32> <i32 poison, i32 0, i32 0, i32 0>, i32 %sum.141, i64 0
  br label %vector.body

vector.body:                                      ; preds = %vector.body, %vector.ph
  %index = phi i32 [ 0, %vector.ph ], [ %index.next, %vector.body ]
  %vec.ind = phi <4 x i32> [ <i32 0, i32 1, i32 2, i32 3>, %vector.ph ], [ %vec.ind.next, %vector.body ]
  %vec.phi = phi <4 x i32> [ %0, %vector.ph ], [ %9, %vector.body ]
  %vec.phi49 = phi <4 x i32> [ zeroinitializer, %vector.ph ], [ %10, %vector.body ]
  %step.add = add <4 x i32> %vec.ind, <i32 4, i32 4, i32 4, i32 4>
  %1 = and <4 x i32> %vec.ind, <i32 1, i32 1, i32 1, i32 1>
  %2 = and <4 x i32> %vec.ind, <i32 1, i32 1, i32 1, i32 1>
  %3 = icmp eq <4 x i32> %1, zeroinitializer
  %4 = icmp eq <4 x i32> %2, zeroinitializer
  %5 = sub nsw <4 x i32> zeroinitializer, %vec.ind
  %6 = sub <4 x i32> <i32 -4, i32 -4, i32 -4, i32 -4>, %vec.ind
  %7 = select <4 x i1> %3, <4 x i32> %vec.ind, <4 x i32> %5
  %8 = select <4 x i1> %4, <4 x i32> %step.add, <4 x i32> %6
  %9 = add <4 x i32> %7, %vec.phi
  %10 = add <4 x i32> %8, %vec.phi49
  %index.next = add nuw i32 %index, 8
  %vec.ind.next = add <4 x i32> %vec.ind, <i32 8, i32 8, i32 8, i32 8>
  %11 = icmp eq i32 %index.next, 1000
  br i1 %11, label %middle.block, label %vector.body, !llvm.loop !5

middle.block:                                     ; preds = %vector.body
  %bin.rdx = add <4 x i32> %10, %9
  %12 = tail call i32 @llvm.vector.reduce.add.v4i32(<4 x i32> %bin.rdx)
  %inc21 = add nuw nsw i16 %j.140, 1
  %exitcond46.not = icmp eq i16 %inc21, 100
  br i1 %exitcond46.not, label %for.inc23, label %vector.ph, !llvm.loop !9

for.inc23:                                        ; preds = %middle.block, %for.body
  %sum.4 = phi i32 [ %sum.044, %for.body ], [ %12, %middle.block ]
  %inc24 = add nuw nsw i16 %k.043, 1
  %exitcond47.not = icmp eq i16 %inc24, 10001
  br i1 %exitcond47.not, label %for.end25, label %for.body, !llvm.loop !10

for.end25:                                        ; preds = %for.inc23
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
!4 = !{!"clang version 16.0.6 (git@github.com:karouzakisp/llvm-project.git 486ba507f059d3e08eb77d7baec3921bd124cf8b)"}
!5 = distinct !{!5, !6, !7, !8}
!6 = !{!"llvm.loop.mustprogress"}
!7 = !{!"llvm.loop.isvectorized", i32 1}
!8 = !{!"llvm.loop.unroll.runtime.disable"}
!9 = distinct !{!9, !6}
!10 = distinct !{!10, !6}
