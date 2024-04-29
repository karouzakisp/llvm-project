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
  switch i32 %indvars.iv, label %for.cond12.preheader [
    i32 101, label %for.inc34
    i32 1005, label %for.inc34
  ]

for.cond12.preheader:                             ; preds = %for.body, %for.inc31.1
  %sum.156 = phi i32 [ %sum.3.1.1, %for.inc31.1 ], [ %sum.060, %for.body ]
  %j.155 = phi i16 [ %inc32.1, %for.inc31.1 ], [ 0, %for.body ]
  %0 = zext i16 %j.155 to i32
  %tobool26 = icmp ne i32 %indvars.iv, %0
  %conv28.neg = sext i1 %tobool26 to i32
  br label %for.body15

for.body15:                                       ; preds = %for.body15, %for.cond12.preheader
  %sum.254 = phi i32 [ %sum.156, %for.cond12.preheader ], [ %sum.3.1, %for.body15 ]
  %i.253 = phi i16 [ 0, %for.cond12.preheader ], [ %inc.1, %for.body15 ]
  %tobool = icmp ne i16 %i.253, 0
  %conv20 = zext i1 %tobool to i32
  %sum.3 = add i32 %sum.254, %conv20
  %sum.3.1 = add i32 %sum.3, %conv28.neg
  %inc.1 = add nuw nsw i16 %i.253, 2
  %exitcond.not.1 = icmp eq i16 %inc.1, 1000
  br i1 %exitcond.not.1, label %for.inc31, label %for.body15, !llvm.loop !5

for.inc31:                                        ; preds = %for.body15
  %inc32 = or i16 %j.155, 1
  %1 = zext i16 %inc32 to i32
  %tobool26.1 = icmp ne i32 %indvars.iv, %1
  %conv28.neg.1 = sext i1 %tobool26.1 to i32
  br label %for.body15.1

for.body15.1:                                     ; preds = %for.body15.1, %for.inc31
  %sum.254.1 = phi i32 [ %sum.3.1, %for.inc31 ], [ %sum.3.1.1, %for.body15.1 ]
  %i.253.1 = phi i16 [ 0, %for.inc31 ], [ %inc.1.1, %for.body15.1 ]
  %tobool.1 = icmp ne i16 %i.253.1, 0
  %conv20.1 = zext i1 %tobool.1 to i32
  %sum.3.165 = add i32 %sum.254.1, %conv20.1
  %sum.3.1.1 = add i32 %sum.3.165, %conv28.neg.1
  %inc.1.1 = add nuw nsw i16 %i.253.1, 2
  %exitcond.not.1.1 = icmp eq i16 %inc.1.1, 1000
  br i1 %exitcond.not.1.1, label %for.inc31.1, label %for.body15.1, !llvm.loop !5

for.inc31.1:                                      ; preds = %for.body15.1
  %inc32.1 = add nuw nsw i16 %j.155, 2
  %exitcond62.not.1 = icmp eq i16 %inc32.1, 100
  br i1 %exitcond62.not.1, label %for.inc34, label %for.cond12.preheader, !llvm.loop !7

for.inc34:                                        ; preds = %for.inc31.1, %for.body, %for.body
  %sum.4 = phi i32 [ %sum.060, %for.body ], [ %sum.060, %for.body ], [ %sum.3.1.1, %for.inc31.1 ]
  %indvars.iv.next = add nuw nsw i32 %indvars.iv, 1
  %exitcond64.not = icmp eq i32 %indvars.iv.next, 50
  br i1 %exitcond64.not, label %for.end36, label %for.body, !llvm.loop !8

for.end36:                                        ; preds = %for.inc34
  %call = tail call i32 (ptr, ...) @printf(ptr noundef nonnull dereferenceable(1) @.str, i32 noundef %sum.4)
  ret i32 0
}

; Function Attrs: nofree nounwind
declare noundef i32 @printf(ptr nocapture noundef readonly, ...) local_unnamed_addr #1

attributes #0 = { mustprogress nofree norecurse nounwind uwtable "min-legal-vector-width"="0" "no-trapping-math"="true" "prefer-vector-width"="1" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #1 = { nofree nounwind "no-trapping-math"="true" "prefer-vector-width"="1" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }

!llvm.module.flags = !{!0, !1, !2, !3}
!llvm.ident = !{!4}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 8, !"PIC Level", i32 2}
!2 = !{i32 7, !"PIE Level", i32 2}
!3 = !{i32 7, !"uwtable", i32 2}
!4 = !{!"clang version 16.0.6 (git@github.com:karouzakisp/llvm-project.git 260e3bfdae174fe14f621ebe8662ac0936673459)"}
!5 = distinct !{!5, !6}
!6 = !{!"llvm.loop.mustprogress"}
!7 = distinct !{!7, !6}
!8 = distinct !{!8, !6}
