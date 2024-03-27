; ModuleID = 'mytest.cpp'
source_filename = "mytest.cpp"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

@.str = private unnamed_addr constant [11 x i8] c"Sum is %d\0A\00", align 1

; Function Attrs: mustprogress nofree norecurse nounwind uwtable
define dso_local noundef i32 @main() local_unnamed_addr #0 {
entry:
  br label %vector.ph

vector.ph:                                        ; preds = %middle.block, %entry
  %sum.022 = phi i32 [ 0, %entry ], [ %12, %middle.block ]
  %j.021 = phi i16 [ 0, %entry ], [ %inc10, %middle.block ]
  %0 = insertelement <4 x i32> <i32 poison, i32 0, i32 0, i32 0>, i32 %sum.022, i64 0
  br label %vector.body

vector.body:                                      ; preds = %vector.body, %vector.ph
  %index = phi i32 [ 0, %vector.ph ], [ %index.next, %vector.body ]
  %vec.ind = phi <4 x i32> [ <i32 0, i32 1, i32 2, i32 3>, %vector.ph ], [ %vec.ind.next, %vector.body ]
  %vec.phi = phi <4 x i32> [ %0, %vector.ph ], [ %9, %vector.body ]
  %vec.phi26 = phi <4 x i32> [ zeroinitializer, %vector.ph ], [ %10, %vector.body ]
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
  %10 = add <4 x i32> %8, %vec.phi26
  %index.next = add nuw i32 %index, 8
  %vec.ind.next = add <4 x i32> %vec.ind, <i32 8, i32 8, i32 8, i32 8>
  %11 = icmp eq i32 %index.next, 1000
  br i1 %11, label %middle.block, label %vector.body, !llvm.loop !5

middle.block:                                     ; preds = %vector.body
  %bin.rdx = add <4 x i32> %10, %9
  %12 = tail call i32 @llvm.vector.reduce.add.v4i32(<4 x i32> %bin.rdx)
  %inc10 = add nuw nsw i16 %j.021, 1
  %exitcond24.not = icmp eq i16 %inc10, 100
  br i1 %exitcond24.not, label %for.end11, label %vector.ph, !llvm.loop !9

for.end11:                                        ; preds = %middle.block
  %call = tail call i32 (ptr, ...) @printf(ptr noundef nonnull dereferenceable(1) @.str, i32 noundef %12)
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
!4 = !{!"clang version 16.0.6 (git@github.com:karouzakisp/llvm-project.git 41a95c5c390d1c2854d15f0ee9cd7ca37c4b6dcf)"}
!5 = distinct !{!5, !6, !7, !8}
!6 = !{!"llvm.loop.mustprogress"}
!7 = !{!"llvm.loop.isvectorized", i32 1}
!8 = !{!"llvm.loop.unroll.runtime.disable"}
!9 = distinct !{!9, !6}
