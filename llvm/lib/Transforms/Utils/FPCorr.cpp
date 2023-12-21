//===-- FPCorr.cpp - FP Correction Transformations --------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "llvm/IR/Operator.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Transforms/Utils/FPCorr.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/TargetSelect.h"

#include <mpfr.h>
#include <mpreal.h>
#include <cassert>


using namespace llvm;

bool isFloatingPointOperation(const Instruction *Instr) {
  Type *Type = Instr->getType();
  if(Type->isFPOrFPVectorTy() ){
    return true;
  }
  for (const Value *Op : Instr->operands()) {
    if (Op->getType()->isFPOrFPVectorTy()) {
        return true; // Found a floating-point operand
    }
  }
  return false;
}

bool isMathOrCall(Instruction *I){
  if (isa<BinaryOperator>(I)){
    BinaryOperator *BinOp = dyn_cast<BinaryOperator>(I);
    if(BinOp->getOpcode() == Instruction::FAdd ||
      BinOp->getOpcode() == Instruction::FSub  ||
      BinOp->getOpcode() == Instruction::FMul ||
      BinOp->getOpcode() == Instruction::FDiv ||
      BinOp->getOpcode() == Instruction::FRem )
      return true;
  }else if (isa<CallInst>(I)) {
    CallInst *Ci = dyn_cast<CallInst>(I);
    Function *F = Ci->getCalledFunction();
    if( F && (F->isIntrinsic() || F->isTargetIntrinsic() || F->isConstrainedFPIntrinsic()  )){
      return false;
    }
    return true; 
  }
  else{
    return false;
  }
  return false; 
}

void visitInstructions(Instruction *I){
      
  for (Use &Op : I->operands()){
    if(auto *Operand = dyn_cast<Instruction>(Op)){
      if(isMathOrCall(Operand)){
        dbgs() << "Visited " << Operand->getOpcode() << "\n";
        visitInstructions(Operand); 
      } 
    }
  }
}

void getAllExpressions(Function &F){
	
	for (inst_iterator It = inst_begin(F), E = inst_end(F); It != E; ++It){
    Instruction *I = &*It;
    if(isFloatingPointOperation(I)){     
      visitInstructions(I); 
    }            
  }   

}

double executeFPFunction(Function &F){
	Function *VisitedF = &F;
/*
	Type *RetTy = VisitedF->getReturnType();
	assert(RetTy->isFloatingPointTy() && "Function does not return Floating Point Type!");
	LLVMContext &Ctx = VisitedF->getContext();

	FunctionType *FTy = VisitedF->getFunctionType();

	std::unique_ptr<Module> module = std::make_unique<Module>("CallFunctionModule", Ctx);
	FunctionCallee callee = module->getOrInsertFunction(VisitedF->getName(), FTy);
	
	InitializeNativeTarget();
	InitializeNativeTargetAsmPrinter();
	ExecutionEngine *EE = EngineBuilder(std::move(module)).create();
*/	
	return 0;
}


PreservedAnalyses FPCorrPass::run(Function &F,
                                      FunctionAnalysisManager &AM) {
 	 
  return PreservedAnalyses::all();
}
