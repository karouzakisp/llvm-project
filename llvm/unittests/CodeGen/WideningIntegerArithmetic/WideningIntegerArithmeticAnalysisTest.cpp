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

} // namespace
