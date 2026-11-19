//===- WideningIntegerArithmeticAnalysisTest.cpp --------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "../../../lib/CodeGen/WideningIntegerArithmetic.cpp"
#include "llvm/Analysis/TargetLibraryInfo.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/NoFolder.h"
#include "llvm/MC/TargetRegistry.h"
#include "gtest/gtest.h"

using namespace llvm;

namespace {

using WIA = WideningIntegerArithmetic;
using Solution = WideningIntegerSolutionInfo;

bool noKnownSignExtension(Value *, unsigned) { return false; }

class WIAAnalysisTest : public testing::Test {
protected:
  LLVMContext Context;
  DataLayout Layout{"e-p:64:64-p1:32:32"};
  Module TestModule{"wia", Context};
  IRBuilder<NoFolder> Builder{TestModule};
  WIA Analysis;

  WIAAnalysisTest() { Analysis.DL = &Layout; }

  Function *body(Type *ReturnType, ArrayRef<Type *> Arguments = {}) {
    auto *F = Function::Create(FunctionType::get(ReturnType, Arguments, false),
                               Function::ExternalLinkage, "body", TestModule);
    Builder.SetInsertPoint(BasicBlock::Create(Context, "entry", F));
    return F;
  }

  Function *callee(Type *ReturnType, ArrayRef<Type *> Arguments = {}) {
    return Function::Create(FunctionType::get(ReturnType, Arguments, false),
                            Function::ExternalLinkage, "callee", TestModule);
  }

  auto &solutions() { return Analysis.AvailableSolutions; }

  void initializeWorklistState(ArrayRef<Value *> Values) {
    unsigned Count = 0;
    for (Value *V : Values)
      Analysis.ValueNum[V] = ++Count;
    Analysis.InsideWorklist.resize(Count + 1);
    Analysis.SolvedInstructions.resize(Count + 1);
  }

  Solution *makeSolution(Value *V, unsigned short PayloadWidth,
                         InstructionCost Cost) {
    return Analysis.SolutionAllocator.allocateSolution<Solution>(
        Instruction::Add, Instruction::Add, SIGN, PayloadWidth, 8, 64, Cost,
        WIAK_UNKNOWN, V);
  }

  void withPlanner(Function &F, function_ref<void(ExtRewritePlanner &)> Check) {
    AssumptionCache Assumptions(F);
    DominatorTree Dominators(F);
    LoopInfo Loops(Dominators);

    DemandedBits Demanded(F, Assumptions, Dominators);

    auto IsNonNegative = [&](Value *V) { return Analysis.isKnownNonNeg(V); };

    WIAInfo Info{};
    Info.IsKnownNonNeg = IsNonNegative;

    ExtRewritePlanner Planner(Demanded, nullptr, nullptr, &Layout, &Loops, Info,
                              Context, false, nullptr);

    Check(Planner);
  }
};

TEST_F(WIAAnalysisTest, AddNonRedundantPruneBadSols) {
  auto *V = Builder.getInt8(1);
  auto *LowerCost = makeSolution(V, 8, 1);
  auto *LowerPayload = makeSolution(V, 4, 3);
  auto *Better = makeSolution(V, 4, 1);
  ASSERT_TRUE(Analysis.addNonRedundant(V, LowerCost));
  ASSERT_TRUE(Analysis.addNonRedundant(V, LowerPayload));
  ASSERT_EQ(solutions().at(V).size(), 2u);
  EXPECT_TRUE(Analysis.addNonRedundant(V, Better));
  ASSERT_EQ(solutions().at(V).size(), 1u);
  EXPECT_EQ(solutions().at(V).front(), Better);
  EXPECT_FALSE(Analysis.addNonRedundant(V, makeSolution(V, 4, 1)));
  EXPECT_FALSE(Analysis.addNonRedundant(V, makeSolution(V, 8, 3)));
  EXPECT_EQ(solutions().at(V).front(), Better);
}

template <unsigned InlineStorage> class TrackedSolution : public Solution {
  SmallVector<unsigned, InlineStorage> Storage;
  SmallVectorImpl<unsigned> &Destroyed;
  unsigned ID;

public:
  TrackedSolution(SmallVectorImpl<unsigned> &Destroyed, unsigned ID, Value *V,
                  unsigned short Payload = 8, InstructionCost Cost = 1)
      : Solution(Instruction::Add, Instruction::Add, SIGN, Payload, 8, 64, Cost,
                 WIAK_UNKNOWN, V),
        Storage(InlineStorage + 1, ID), Destroyed(Destroyed), ID(ID) {}

  ~TrackedSolution() { Destroyed.push_back(ID); }
};

TEST(WIASolutionArenaTest, TestArenaReset) {
  LLVMContext Context;
  auto *V = ConstantInt::get(Type::getInt8Ty(Context), 1);
  SmallVector<unsigned, 4> Destroyed;
  {
    SolutionArena Arena;
    Arena.allocateSolution<TrackedSolution<0>>(Destroyed, 1, V);
    Arena.allocateSolution<TrackedSolution<4>>(Destroyed, 2, V);
    Arena.allocateSolution<TrackedSolution<0>>(Destroyed, 3, V);
    EXPECT_TRUE(Destroyed.empty());
    Arena.Reset();
    EXPECT_EQ(Destroyed, (SmallVector<unsigned, 4>{3, 2, 1}));
    Arena.Reset();
    EXPECT_EQ(Destroyed, (SmallVector<unsigned, 4>{3, 2, 1}));
    Arena.allocateSolution<TrackedSolution<4>>(Destroyed, 4, V);
  }
  EXPECT_EQ(Destroyed, (SmallVector<unsigned, 4>{3, 2, 1, 4}));
}

TEST_F(WIAAnalysisTest, RepeatedUndefVisitsKeepsSols) {

  auto *V = UndefValue::get(Builder.getInt8Ty());
  Analysis.visitUndef(V);
  ASSERT_FALSE(solutions().at(V).empty());
  const auto Sols = solutions().at(V);
  // Calling visit undef second time shouldn't change anything.
  Analysis.visitUndef(V);
  EXPECT_EQ(Sols, solutions().at(V));
}

TEST_F(WIAAnalysisTest, MaskedValueIsKnownNonNegative) {
  auto *F = body(Builder.getVoidTy(), {Builder.getInt16Ty()});
  auto *X = F->getArg(0);

  auto *NonNeg = cast<Instruction>(Builder.CreateAnd(X, Builder.getInt16(255)));
  // set the sign bit to 1 to create a negative value.
  auto *Negative =
      cast<Instruction>(Builder.CreateOr(X, Builder.getInt16(0x8000)));
  EXPECT_TRUE(Analysis.isKnownNonNeg(NonNeg));
  EXPECT_FALSE(Analysis.isKnownNonNeg(Negative));

  Builder.CreateRetVoid();
}

TEST_F(WIAAnalysisTest, SatisfyResultFillTest) {
  auto *F = body(Builder.getVoidTy(), {Builder.getInt16Ty()});
  auto *X = F->getArg(0);

  auto *NonNeg = cast<Instruction>(Builder.CreateAnd(X, Builder.getInt16(255)));
  // set the sign bit to 1 to create a negative value.
  auto *Negative =
      cast<Instruction>(Builder.CreateOr(X, Builder.getInt16(0x8000)));
  // if we have NonNeg and SIGN then this can be ZEROS too, so
  // so the resultFill is Satisfied.
  EXPECT_TRUE(Analysis.satisfiesResultFill(NonNeg, SIGN, ZEROS));
  // Otherwise, is not satisfied.
  EXPECT_FALSE(Analysis.satisfiesResultFill(Negative, SIGN, ZEROS));

  // If we need a SIGN even if we have a negative value that can work.
  EXPECT_TRUE(Analysis.satisfiesResultFill(Negative, SIGN, SIGN));

  // This is true because SIGN copies the sign bit, if this is non negative
  // the sign bit is guaranteed to be non zero, so it matches with ZEROS.
  EXPECT_TRUE(Analysis.satisfiesResultFill(NonNeg, ZEROS, SIGN));
  // Otherwise, is not satisfied.
  EXPECT_FALSE(Analysis.satisfiesResultFill(Negative, ZEROS, SIGN));
  Builder.CreateRetVoid();
}

TEST_F(WIAAnalysisTest, SolvedForWidthTest) {

  auto *V = Builder.getInt8(7);
  EXPECT_FALSE(Analysis.isSolvedForWidth(V, 64));

  auto *Sol = makeSolution(V, 8, 1);

  EXPECT_TRUE(Analysis.addNonRedundant(V, Sol));

  // makeSolution hardcodes the updated width to 64, so V
  // has a solution with Updatedwidth = 64.
  EXPECT_TRUE(Analysis.isSolvedForWidth(V, 64));

  EXPECT_FALSE(Analysis.isSolvedForWidth(V, 32));
}

TEST_F(WIAAnalysisTest, BuildWorklistRegion) {
  auto *I8 = Builder.getInt8Ty();
  auto *F = body(Builder.getInt32Ty(), {I8, I8, I8});

  auto *A = F->getArg(0);
  auto *B = F->getArg(1);
  auto *C = F->getArg(2);

  auto *Sum = Builder.CreateAdd(A, B, "sum");
  auto *NonRelated = Builder.CreateAdd(C, C, "unrelated");
  auto *Ext = Builder.CreateSExt(Sum, Builder.getInt32Ty(), "ext");
  auto *Ret = Builder.CreateRet(Ext);

  SmallVector<Value *, 8> Worklist;
  Analysis.buildWorklist(*F, Worklist);

  ASSERT_EQ(Worklist.size(), 3u);

  EXPECT_TRUE(llvm::is_contained(Worklist, Sum));
  EXPECT_TRUE(llvm::is_contained(Worklist, Ext));
  EXPECT_TRUE(llvm::is_contained(Worklist, Ret));

  EXPECT_FALSE(llvm::is_contained(Worklist, NonRelated));

  // worklist is being build in reverse so Sum is in the back.
  EXPECT_EQ(Worklist.back(), Sum);
}

TEST_F(WIAAnalysisTest, WorklistTest) {
  auto *I8 = Builder.getInt8Ty();
  auto *F = body(Builder.getInt32Ty(), {I8, I8});

  auto *A = F->getArg(0);
  auto *B = F->getArg(1);

  auto *Sum = Builder.CreateAdd(A, B, "sum");
  auto *Ext = Builder.CreateSExt(Sum, Builder.getInt32Ty(), "ext");
  Builder.CreateRet(Ext);

  SmallVector<Value *, 8> Worklist;
  Analysis.buildWorklist(*F, Worklist);

  // Simulate having processed everything already queued.
  Worklist.clear();
  Analysis.InsideWorklist.reset();

  // Mark Ext as solved.
  Analysis.SolvedInstructions.set(Analysis.ValueToNum(Ext));
  ASSERT_TRUE(Analysis.isSolved(Ext));

  Analysis.pushUsersToWorklist(Sum, Worklist);

  ASSERT_TRUE(Analysis.isInWorklist(Ext));
  ASSERT_FALSE(Analysis.isSolved(Ext));
  ASSERT_EQ(Worklist.size(), 1u);
  EXPECT_EQ(Worklist.back(), Ext);

  Analysis.pushUsersToWorklist(Sum, Worklist);

  // Worklist prevent duplicates, so size must remain 1.
  ASSERT_EQ(Worklist.size(), 1u);
}

TEST_F(WIAAnalysisTest, AssignOpFillsTest) {
  auto *F = body(Builder.getVoidTy(), {Builder.getInt1Ty(), Builder.getInt8Ty(),
                                       Builder.getInt8Ty()});

  auto Select = cast<Instruction>(
      Builder.CreateSelect(F->getArg(0), F->getArg(1), F->getArg(2)));
  Builder.CreateRetVoid();

  SmallVector<IntegerFillType, 3> Fills;
  assignOpFills(Select, SIGN, ZEROS, Fills);

  ASSERT_EQ(Fills[0], ANYTHING);
  ASSERT_EQ(Fills[1], SIGN);
  ASSERT_EQ(Fills[2], ZEROS);
}

  TEST_F(WIAAnalysisTest, NoWrapFillswithConstantsTest) {
    auto *F = body(Builder.getVoidTy(), {Builder.getInt8Ty(), Builder.getInt8Ty(),
                                       Builder.getInt8Ty()});
    Value *X = F->getArg(0);

    auto *Mask = Builder.CreateAnd(X, Builder.getInt8(15));

    auto *SafeAdd = cast<Instruction>(Builder.CreateAdd(Mask, Builder.getInt8(1)));
    auto *Unknown = cast<Instruction>(Builder.CreateAdd(X, Builder.getInt8(1)));

    // Safe
    ASSERT_EQ(Analysis.tryToProveNoWrapFlagsWithConstant(SafeAdd, ZEROS),
              ZEROS);

    ASSERT_EQ(Analysis.tryToProveNoWrapFlagsWithConstant(Unknown, ZEROS),
              UNDEFINED);

    Builder.CreateRetVoid();

  }

  TEST_F(WIAAnalysisTest, SignedOperandsCanProduceZeroFill) {
  body(Builder.getVoidTy());

  auto *Positive = cast<Instruction>(
      Builder.CreateAdd(Builder.getInt8(127), Builder.getInt8(1)));

  auto *Negative = cast<Instruction>(
      Builder.CreateAdd(Builder.getInt8(-1), Builder.getInt8(0)));

  Builder.CreateRetVoid();

  EXPECT_TRUE(Analysis.canProduceZeroFillFromSignedOps(Positive));
  EXPECT_FALSE(Analysis.canProduceZeroFillFromSignedOps(Negative));

}

  TEST_F(WIAAnalysisTest, SwitchClampsNegativeCost) {
  auto *F = body(Builder.getVoidTy(), {Builder.getInt8Ty()});
  auto *Condition = F->getArg(0);
  auto *Exit = BasicBlock::Create(Context, "exit", F);
  auto *Switch = Builder.CreateSwitch(Condition, Exit);
  Switch->addCase(Builder.getInt8(1), Exit);
  Builder.SetInsertPoint(Exit);
  Builder.CreateRetVoid();

  Analysis.ExtensionChoice = SIGN;
  auto *CondSol = makeSolution(Condition, 8, -2);
  solutions()[Condition].push_back(CondSol);

  SmallVector<Value *, 4> Worklist;
  Analysis.visitSwitch(Switch, Worklist);

  const auto &Sols = solutions().at(Switch);
  ASSERT_EQ(Sols.size(), 1u);

  auto *Sol = Sols.front();
  EXPECT_EQ(Sol->getCost(), InstructionCost(0));
}

  TEST_F(WIAAnalysisTest, RegisterSolutionsReportsOnlyAcceptedCandidates) {
  auto *F = body(Builder.getVoidTy(), {Builder.getInt8Ty()});
  auto *I = cast<Instruction>(
      Builder.CreateAdd(F->getArg(0), Builder.getInt8(0)));
  Builder.CreateRetVoid();

  // Keep payload, updated width and fill identical; vary only the cost.
  auto *Existing = makeSolution(I, 8, 2);
  auto *Worse = makeSolution(I, 8, 3);
  auto *Better = makeSolution(I, 8, 1);
  solutions()[I].push_back(Existing);

  SmallVector<Solution *, 8> Accepted;
  ASSERT_TRUE(
      Analysis.tryToRegisterSolutions(I, {Worse, Better}, Accepted));

  const auto &Sols = solutions().at(I);

  // Only The 'better' solution must be registered.
  ASSERT_EQ(Accepted.size(), 1u);
  EXPECT_EQ(Accepted.front(), Better);

  // Similarly in the available solutions too.
  ASSERT_EQ(Sols.size(), 1u);
  EXPECT_EQ(Sols.front(), Better);
  Accepted.clear();

  // Registering the same solution produces no changes.
  EXPECT_FALSE(
    Analysis.tryToRegisterSolutions(I, {Better}, Accepted));
  EXPECT_TRUE(Accepted.empty());
  ASSERT_EQ(Sols.size(), 1u);
  EXPECT_EQ(Sols.front(), Better);
}

  TEST_F(WIAAnalysisTest, CombineSolsNeedsBothOperands) {
  auto *F =
      body(Builder.getVoidTy(), {Builder.getInt8Ty(), Builder.getInt8Ty()});
  auto *Add = cast<Instruction>(
      Builder.CreateAdd(F->getArg(0), F->getArg(1)));
  Builder.CreateRetVoid();

  // Only the left operand has a candidate.
  solutions()[F->getArg(0)].push_back(makeSolution(F->getArg(0), 8, 1));

  SmallVector<Value *, 4> Worklist;
  EXPECT_FALSE(Analysis.combineSols(Add, Worklist));

  // No result candidate should have been created.
  EXPECT_EQ(solutions().count(Add), 0u);
}

  TEST_F(WIAAnalysisTest, KeepSExtWithoutNSW) {
  auto *F = body(Builder.getVoidTy(), {Builder.getInt8Ty()});

  // This add has no nsw flag, so signed overflow is possible.
  auto *Add = cast<Instruction>(
      Builder.CreateAdd(F->getArg(0), Builder.getInt8(1)));
  auto *Ext = cast<Instruction>(
      Builder.CreateSExt(Add, Builder.getInt32Ty()));
  Builder.CreateRetVoid();

  Analysis.Ctx = &Context;
  Analysis.ExtensionChoice = SIGN;

  // makeSolution defaults to updated width 64.
  // change it to i8
  auto *Default = makeSolution(Add, 8, 0);
  Default->setUpdatedWidth(8);
  solutions()[Add].push_back(Default);

  SmallVector<Value *, 4> Worklist;
  EXPECT_FALSE(Analysis.visitDropExt(Ext, Worklist));

  // Keep the extension instead of generating a DROP_EXT candidate.
  const auto &Sols = solutions().at(Ext);
  ASSERT_EQ(Sols.size(), 1u);
  auto *S = Sols.front();
  // Without nsw on the add, keep the sext,
  EXPECT_EQ(S->getKind(), WIAK_NOOP);
  EXPECT_EQ(S->getUpdatedWidth(), 32u);

  ASSERT_EQ(S->getOperands().size(), 1u);
  EXPECT_EQ(S->getOperand(0), Default);
}

TEST_F(WIAAnalysisTest, TruncWaitsForOperand) {
  auto *F = body(Builder.getInt8Ty(), {Builder.getInt32Ty()});
  auto *X = F->getArg(0);
  auto *Trunc = cast<Instruction>(Builder.CreateTrunc(X, Builder.getInt8Ty()));
  Builder.CreateRet(Trunc);

  SmallVector<Value *, 8> Worklist;
  // A trunc alone is not a discovery seed; initialize this isolated visitor.
  initializeWorklistState({X, Trunc});

  // X has no candidate yet.
  EXPECT_FALSE(Analysis.visitDropTrunc(Trunc, Worklist));

  ASSERT_EQ(Worklist.size(), 1u);
  EXPECT_EQ(Worklist.back(), X);

  // No sol was created rot runc
  EXPECT_EQ(solutions().count(Trunc), 0u);
}

TEST_F(WIAAnalysisTest, PromotionNeedsMatchingWidthAndFill) {
  auto *F = body(Builder.getVoidTy(), {Builder.getInt8Ty()});
  auto *X = F->getArg(0);
  Builder.CreateRetVoid();

  // No sols exists yet.
  EXPECT_FALSE(Analysis.hasPromotionSolution(X, 64, SIGN));

  // makeSolution creates an i64 candidate with SIGN fill.
  auto *S = makeSolution(X, 8, 0);
  solutions()[X].push_back(S);

  EXPECT_TRUE(Analysis.hasPromotionSolution(X, 64, SIGN));

  // A different width or concrete fill must not match.
  EXPECT_FALSE(Analysis.hasPromotionSolution(X, 32, SIGN));
  EXPECT_FALSE(Analysis.hasPromotionSolution(X, 64, ZEROS));

  // ANYTHING accepts this candidate; fill constraints are checked later.
  EXPECT_TRUE(Analysis.hasPromotionSolution(X, 64, ANYTHING));

  S->setFillType(UNDEFINED);
  EXPECT_FALSE(Analysis.hasPromotionSolution(X, 64, ANYTHING));
}

TEST_F(WIAAnalysisTest, TruncBoundaries) {
  auto *F = body(Builder.getInt8Ty(), {Builder.getInt8Ty()});
  auto *X = F->getArg(0);

  auto *Ext = cast<CastInst>(Builder.CreateSExt(X, Builder.getInt64Ty()));
  auto *Trunc = cast<TruncInst>(Builder.CreateTrunc(Ext, Builder.getInt8Ty()));
  Builder.CreateRet(Trunc);

  withPlanner(*F, [&](ExtRewritePlanner &Planner) {
    ExtRewriteGraph G;
    G.Ext = Ext;
    G.Source = Ext->getOperand(0);
    G.NarrowWidth = 8;
    G.WideWidth = 64;

    bool Accept =
        Planner.handleTruncBoundary(Trunc->getOperandUse(0), Trunc, G);
    EXPECT_TRUE(Accept);

    ASSERT_EQ(G.BoundaryRewrites.size(), 1u);

    EXPECT_EQ(G.BoundaryRewrites.front().Kind,
              BoundaryRewriteKind::ReplaceWithSource);
  });
}

TEST_F(WIAAnalysisTest, NarrowShiftNodes) {
  auto *F = body(Builder.getInt8Ty(), {Builder.getInt8Ty()});
  auto *X = F->getArg(0);

  auto *Ext = cast<CastInst>(Builder.CreateZExt(X, Builder.getInt64Ty()));
  auto *GoodShift =
      cast<Instruction>(Builder.CreateShl(Ext, Builder.getInt64(3)));

  auto *BadShift =
      cast<Instruction>(Builder.CreateShl(Ext, Builder.getInt64(8)));
  auto *Trunc = Builder.CreateTrunc(GoodShift, Builder.getInt8Ty());
  Builder.CreateRet(Trunc);

  withPlanner(*F, [&](ExtRewritePlanner &Planner) {
    ExtRewriteGraph G;
    G.Ext = Ext;
    G.Source = Ext->getOperand(0);
    G.NarrowWidth = 8;
    G.WideWidth = 64;

    SmallVector<Value *, 8> Worklist;

    // A shift by 3 is valid at the proposed i8 width.
    EXPECT_TRUE(Planner.tryToAddNarrowGraphNode(GoodShift, G, Worklist));

    // The accepted instruction should enter the graph and worklist.
    EXPECT_TRUE(G.Nodes.contains(GoodShift));

    EXPECT_FALSE(Planner.tryToAddNarrowGraphNode(BadShift, G, Worklist));
    EXPECT_FALSE(G.Nodes.contains(BadShift));

    ASSERT_EQ(Worklist.size(), 1u);
    EXPECT_EQ(Worklist.back(), GoodShift);

    EXPECT_FALSE(G.Visiting.contains(GoodShift));
  });
}

TEST_F(WIAAnalysisTest, TruncRootCollectsAddAndExit) {
  // trunc i64 -> i8 -> add -> sext back to i64
  auto *F = body(Builder.getInt64Ty(), {Builder.getInt64Ty()});
  auto *Trunc =
      cast<TruncInst>(Builder.CreateTrunc(F->getArg(0), Builder.getInt8Ty()));
  auto *Add = cast<Instruction>(Builder.CreateAdd(Trunc, Builder.getInt8(1)));
  auto *Exit = cast<CastInst>(Builder.CreateSExt(Add, Builder.getInt64Ty()));
  Builder.CreateRet(Exit);

  withPlanner(*F, [&](ExtRewritePlanner &Planner) {
    TruncRootGraph G;
    G.Root = Trunc;
    G.NarrowWidth = 8;
    G.WideWidth = 64;

    ASSERT_TRUE(Planner.tryToBuildTruncGraphFromUsers(Trunc, G));

    // The add belongs to the computation that will be widened.
    EXPECT_TRUE(G.Nodes.contains(Add));

    // The extension marks where the narrow computation ends.
    ASSERT_EQ(G.Exits.size(), 1u);
    EXPECT_EQ(G.Exits.front(), Exit);

    // Remove the casts; retain the add for widening.
    EXPECT_TRUE(llvm::is_contained(G.InstsToRemove, Trunc));
    EXPECT_TRUE(llvm::is_contained(G.InstsToRemove, Exit));
    EXPECT_FALSE(llvm::is_contained(G.InstsToRemove, Add));
  });
}

TEST_F(WIAAnalysisTest, SignedClampPlan) {
  auto *F = body(Builder.getInt8Ty(), {Builder.getInt8Ty()});

  auto *Wide = Builder.CreateSExt(F->getArg(0), Builder.getInt64Ty());
  auto *Shift = Builder.CreateLShr(Wide, Builder.getInt64(4));
  auto *Trunc =
      cast<TruncInst>(Builder.CreateTrunc(Shift, Builder.getInt8Ty()));

  auto *Add = Builder.CreateAdd(Trunc, Builder.getInt8(1));
  auto *Low =
      Builder.CreateBinaryIntrinsic(Intrinsic::smax, Add, Builder.getInt8(0));
  auto *High =
      Builder.CreateBinaryIntrinsic(Intrinsic::umin, Low, Builder.getInt8(100));
  Builder.CreateRet(High);

  withPlanner(*F, [&](ExtRewritePlanner &Planner) {
    TruncRootGraph G;
    G.Root = Trunc;
    G.NarrowWidth = 8;
    G.WideWidth = 64;

    auto Plan = Planner.tryToBuildSignedClampPlan(Trunc);
    ASSERT_TRUE(Plan.has_value());

    EXPECT_EQ(Plan->Kind, ExtStrategy::PromoteSource);

    EXPECT_EQ(Plan->NarrowWidth, 8u);
    EXPECT_EQ(Plan->WideWidth, 64u);

    EXPECT_EQ(Plan->TruncToReplaceWithAShr, Trunc);

    EXPECT_TRUE(llvm::is_contained(Plan->InstsToRemove, Trunc));
    EXPECT_TRUE(llvm::is_contained(Plan->InstsToRemove, Shift));
  });
}

TEST_F(WIAAnalysisTest, SignedClampPlanFailTest) {
  auto *F = body(Builder.getInt8Ty(), {Builder.getInt8Ty()});

  auto *Wide = Builder.CreateSExt(F->getArg(0), Builder.getInt64Ty());
  auto *Shift = Builder.CreateLShr(Wide, Builder.getInt64(4));
  auto *Trunc =
      cast<TruncInst>(Builder.CreateTrunc(Shift, Builder.getInt8Ty()));

  auto *Add = Builder.CreateAdd(Trunc, Builder.getInt8(127));
  auto *Low =
      Builder.CreateBinaryIntrinsic(Intrinsic::smax, Add, Builder.getInt8(0));
  auto *High =
      Builder.CreateBinaryIntrinsic(Intrinsic::umin, Low, Builder.getInt8(100));
  Builder.CreateRet(High);

  withPlanner(*F, [&](ExtRewritePlanner &Planner) {
    TruncRootGraph G;
    G.Root = Trunc;
    G.NarrowWidth = 8;
    G.WideWidth = 64;

    auto Plan = Planner.tryToBuildSignedClampPlan(Trunc);
    EXPECT_EQ(Plan, std::nullopt);
  });
}

TEST_F(WIAAnalysisTest, SourcePromoteNodeTest) {
  auto *F =
      body(Builder.getInt64Ty(), {Builder.getInt8Ty(), Builder.getInt8Ty()});
  auto *Add =
      cast<Instruction>(Builder.CreateNSWAdd(F->getArg(0), F->getArg(1)));
  auto *Ext = cast<CastInst>(Builder.CreateSExt(Add, Builder.getInt64Ty()));
  Builder.CreateRet(Ext);

  withPlanner(*F, [&](ExtRewritePlanner &Planner) {
    SourcePromoteGraph G;
    G.Ext = Ext;
    G.Source = Add;
    G.SrcWidth = 8;
    G.DstWidth = 64;
    G.Fill = SIGN;

    ASSERT_TRUE(Planner.tryToAddSourcePromoteNode(Add, G, SIGN));
    ASSERT_TRUE(G.containsNode(Add));
  });

  auto *ExtF = body(Builder.getVoidTy(), {Builder.getInt8Ty()});
  auto *X = ExtF->getArg(0);

  auto *S32 = cast<Instruction>(Builder.CreateSExt(X, Builder.getInt32Ty()));
  auto *Z32 = cast<Instruction>(Builder.CreateZExt(X, Builder.getInt32Ty()));
  auto *S64 = cast<Instruction>(Builder.CreateSExt(X, Builder.getInt64Ty()));
  Builder.CreateRetVoid();

  withPlanner(*ExtF, [&](ExtRewritePlanner &Planner) {
    SourcePromoteGraph G;
    G.DstWidth = 64;

    // 1st case
    ASSERT_TRUE(Planner.tryToAddSourcePromoteNode(S32, G, SIGN));
    EXPECT_TRUE(G.containsNode(S32));
    EXPECT_TRUE(G.InstsToReplaceWithOperand.contains(S32));
    EXPECT_EQ(G.ExternalOpsToExtend.at(X), SIGN);

    G = SourcePromoteGraph{};
    G.DstWidth = 64;

    // 2nd case
    ASSERT_TRUE(Planner.tryToAddSourcePromoteNode(Z32, G, ZEROS));
    EXPECT_TRUE(G.containsNode(Z32));
    EXPECT_TRUE(G.InstsToReplaceWithOperand.contains(Z32));
    EXPECT_EQ(G.ExternalOpsToExtend.at(X), ZEROS);

    G = SourcePromoteGraph{};
    G.DstWidth = 64;

    // 3rd case, S64 has already the target width
    // so it becomes a wide leaf.
    ASSERT_TRUE(Planner.tryToAddSourcePromoteNode(S64, G, SIGN));
    EXPECT_TRUE(G.WideLeaves.contains(S64));
    EXPECT_FALSE(G.InstsToReplaceWithOperand.contains(S64));
    EXPECT_FALSE(G.containsNode(S64));
    EXPECT_TRUE(G.ExternalOpsToExtend.empty());
  });
}

TEST_F(WIAAnalysisTest, MergeFillsTest) {
  auto *F = body(Builder.getVoidTy(), {Builder.getInt16Ty()});
  auto *Unknown = F->getArg(0);
  auto *NonNegative =
      cast<Instruction>(Builder.CreateAnd(Unknown, Builder.getInt16(127)));
  Builder.CreateRetVoid();

  withPlanner(*F, [&](ExtRewritePlanner &Planner) {
    SourcePromoteGraph G;

    // First requirement for this value: record SIGN.
    EXPECT_EQ(Planner.mergeFill(G, NonNegative, SIGN),
              MergeFillResult::Changed);
    EXPECT_EQ(G.getRequiredFill(NonNegative), SIGN);

    // require ZEROS now
    EXPECT_EQ(Planner.mergeFill(G, NonNegative, ZEROS),
              MergeFillResult::Unchanged);
    // Since the result is NonNegative it can stay SIGN.
    EXPECT_EQ(G.getRequiredFill(NonNegative), SIGN);

    EXPECT_EQ(Planner.mergeFill(G, Unknown, SIGN), MergeFillResult::Changed);
    EXPECT_EQ(Planner.mergeFill(G, Unknown, ZEROS), MergeFillResult::Conflict);
    EXPECT_EQ(G.getRequiredFill(Unknown), SIGN);
  });
}

TEST_F(WIAAnalysisTest, SignedWrapConstants) {
  auto *F = body(Builder.getInt1Ty(), {Builder.getInt8Ty()});

  auto *Add =
      cast<Instruction>(Builder.CreateAdd(F->getArg(0), Builder.getInt8(1)));
  auto *Cmp = cast<ICmpInst>(Builder.CreateICmpSLT(Add, Builder.getInt8(0)));
  Builder.CreateRet(Cmp);

  APInt NewOpConstant;
  APInt NewCmpConstant;

  ASSERT_TRUE(computeSignedICmpWrapConstants(Add, Cmp, 8, 64, NewOpConstant,
                                             NewCmpConstant));

  ASSERT_EQ(NewOpConstant.getBitWidth(), 64u);
  ASSERT_EQ(NewCmpConstant.getBitWidth(), 64u);

  EXPECT_EQ(NewOpConstant.getSExtValue(), -127ll);
  EXPECT_EQ(NewCmpConstant.getSExtValue(), -128ll);
}

TEST_F(WIAAnalysisTest, GepSignFillTest) {
  auto *F = body(Builder.getVoidTy(), {Builder.getInt8Ty()});

  // A local 100-byte object.
  auto *ArrayTy = ArrayType::get(Builder.getInt8Ty(), 100);
  auto *Table = new GlobalVariable(
      TestModule, ArrayTy, true, GlobalValue::InternalLinkage,
      ConstantAggregateZero::get(ArrayTy), "table");

  auto *Ext =
      cast<ZExtInst>(Builder.CreateZExt(F->getArg(0), Builder.getInt64Ty()));

  Builder.CreateInBoundsGEP(ArrayTy, Table, {Builder.getInt64(0), Ext});
  Builder.CreateRetVoid();

  EXPECT_TRUE(canUseSignFillForGEP(Ext, 8, Layout));
}

TEST_F(WIAAnalysisTest, GepSignFillNegativeTest) {
  auto *F = body(Builder.getVoidTy(), {Builder.getInt8Ty()});

  // A local 128-byte object.
  auto *ArrayTy = ArrayType::get(Builder.getInt8Ty(), 128);
  auto *Table = new GlobalVariable(
      TestModule, ArrayTy, true, GlobalValue::InternalLinkage,
      ConstantAggregateZero::get(ArrayTy), "table");

  auto *Ext =
      cast<ZExtInst>(Builder.CreateZExt(F->getArg(0), Builder.getInt64Ty()));

  Builder.CreateInBoundsGEP(ArrayTy, Table, {Builder.getInt64(0), Ext});
  Builder.CreateRetVoid();

  // sign range is between 0 and 127, so sext produces negative.
  EXPECT_FALSE(canUseSignFillForGEP(Ext, 8, Layout));
}

TEST_F(WIAAnalysisTest, TestUndoLogAndFills) {
  auto *X = Builder.getInt8(1);
  auto *Y = Builder.getInt8(2);

  DenseMap<Value *, IntegerFillType> Fills;
  Fills[X] = SIGN;

  PromoteConstraintSolver::UndoLog<Value *> Log;
  auto Checkpoint = Log.snapshot();

  Log.set(Fills, X, ZEROS);

  // Add new entry.
  Log.set(Fills, Y, ANYTHING);

  ASSERT_EQ(Fills.size(), 2u);
  ASSERT_EQ(Fills.at(X), ZEROS);

  Log.rollback(Checkpoint);

  EXPECT_EQ(Fills.at(X), SIGN);

  EXPECT_FALSE(Fills.contains(Y));
}

TEST_F(WIAAnalysisTest, PropagateForPendingChoice) {
  auto *F =
      body(Builder.getInt8Ty(), {Builder.getInt8Ty(), Builder.getInt8Ty()});
  auto *And = cast<Instruction>(Builder.CreateAnd(F->getArg(0), F->getArg(1)));
  Builder.CreateRet(And);

  // Simulate multiple choices for operand fills.
  auto ChooseFills = [](Instruction *, IntegerFillType,
                        SmallVectorImpl<IntegerFillType> &Fills,
                        SmallVectorImpl<FillTuple> *GeneratedTuples) {
    Fills.assign(2, PENDING);
    if (GeneratedTuples) {
      GeneratedTuples->clear();
      GeneratedTuples->emplace_back(ZEROS, ANYTHING, ZEROS);
      GeneratedTuples->emplace_back(ANYTHING, ZEROS, ZEROS);
    }
    return true;
  };

  SourcePromoteGraph G;
  G.DstWidth = 64;
  G.addNode(And);

  WIAInfo Info{};
  Info.ChooseOpsFills = ChooseFills;
  WIATuplesMap Tuples;
  PromoteConstraintSolver Solver(G, Info, Layout, Context, noKnownSignExtension,
                                 Tuples);

  DenseMap<Value *, IntegerFillType> ReqFill;
  DenseMap<const Use *, IntegerFillType> ReqOperandFill;
  PromoteConstraintSolver::UndoLog<Value *> ValueLog;
  PromoteConstraintSolver::UndoLog<const Use *> OperandLog;
  SmallSetVector<Value *, 32> Worklist;

  ReqFill[And] = ZEROS;
  Worklist.insert(And);

  auto Result = Solver.propagateUntil(ReqFill, ReqOperandFill, ValueLog,
                                      OperandLog, Worklist);

  EXPECT_EQ(Result.Status,
            PromoteConstraintSolver::PropagationStatus::Continue);
  EXPECT_EQ(Result.I, And);
  EXPECT_EQ(Result.Fill, ZEROS);
}

TEST_F(WIAAnalysisTest, CollectFillReqsTest) {
  auto *F = body(Builder.getInt8Ty(), {Builder.getInt8Ty()});
  auto *X = F->getArg(0);
  auto *Entry = Builder.GetInsertBlock();
  auto *Merge = BasicBlock::Create(Context, "merge", F);
  Builder.CreateBr(Merge);

  Builder.SetInsertPoint(Merge);
  auto *Phi = Builder.CreatePHI(Builder.getInt8Ty(), 1);
  Phi->addIncoming(X, Entry);
  Builder.CreateRet(Phi);

  SourcePromoteGraph G;
  G.Source = Phi;
  G.SrcWidth = 8;
  G.DstWidth = 64;
  G.Fill = SIGN;
  G.addNode(Phi);
  G.ExternalOpsToExtend[X] = SIGN;

  auto ChooseFills = [](Instruction *I, IntegerFillType Fill,
                        SmallVectorImpl<IntegerFillType> &Fills,
                        SmallVectorImpl<FillTuple> *GeneratedTuples) {
    if (GeneratedTuples)
      GeneratedTuples->clear();

    Fills.assign(I->getNumOperands(), Fill);
    return true;
  };

  WIAInfo Info{};
  Info.ChooseOpsFills = ChooseFills;
  WIATuplesMap Tuples;
  PromoteConstraintSolver Solver(G, Info, Layout, Context, noKnownSignExtension,
                                 Tuples);

  bool Solved = Solver.solve();

  ASSERT_TRUE(Solved);

  const auto &Fills = Solver.getRequiredFill();
  EXPECT_EQ(Fills.lookup_or(Phi, UNDEFINED), SIGN);

  auto IsPending = [](const auto &Entry) { return Entry.second == PENDING; };
  EXPECT_FALSE(llvm::any_of(Fills, IsPending));
  EXPECT_FALSE(llvm::any_of(Solver.getRequiredOperandFill(), IsPending));
  EXPECT_FALSE(llvm::any_of(Solver.getLoadsToExtend(), IsPending));
}

} // namespace
