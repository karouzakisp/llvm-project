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
#include "llvm/IR/Module.h"
#include "llvm/IR/Verifier.h"
#include "llvm/IR/Type.h"
#include "llvm/Transforms/Utils/FPCorr.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ExecutionEngine/GenericValue.h"


#include <vector>
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

bool hasGlobalVariables(Function &F){
	for (inst_iterator It = inst_begin(F), E = inst_end(F); It != E; ++It){
    Instruction *I = &*It;
		if (auto *GEP = dyn_cast<GetElementPtrInst>(&I)) {
			if (auto *GV = dyn_cast<GlobalVariable>(GEP->getPointerOperand())) {
				return true;			
			}
		}
	}
	return false;	
}

bool canExecuteF(Function &F){
	if(hasGlobalVariables(F)){
		return false;
	}
}

double executeFPFunction(Function &F){
	Function *VisitedF = &F;
	Type *RetTy = VisitedF->getReturnType();
	assert(RetTy->isFloatingPointTy() && "Function does not return Floating Point Type!");	
	if(hasGlobalVariables(F)){
		return 0;
	}	
	SmallVector<GenericValue, 4> arguments;
	std::vector<Type *> ArgTypes;
	std::vector<StringRef> ArgNames;
	LLVMContext &Ctx = VisitedF->getContext();
	Module *OldModule = VisitedF->getParent();

	FunctionType *FTy = VisitedF->getFunctionType();

	for (Argument &arg : VisitedF->args()) {
		ArgTypes.push_back(arg.getType());
		ArgNames.push_back(arg.getName());
	}

	FunctionType *Fty = VisitedF->getfunctionType();	
	FunctionCallee Callee = OldModule->getOrInsertFunction(VisitedF->getName(), Fty);
	
	
	std::unique_ptr<Module> NewModule = std::make_unique<Module>("MyNewModule", Ctx);
	
	// create the new main function that contains the call	
	FunctionType *MainFTy = FunctionType::get(Type::getInt32Ty(Context), false);
	Function *MainFunc = Function::Create(MainFTy, Function::ExternalLinkage, "main", NewModule.get());
	
	// Create an entry basic block for the main function
	BasicBlock *EntryBlock = BasicBlock::Create(Ctx, "entry", MainFunc);
	IRBuilder<> Builder(EntryBlock);
	
	std::unique_ptr<RandomNumberGenerator> RNG = OldModule->createRNG("FPCorrPass");	

	for (int i = 0; i < 1000; ++i){
		for (size_t i = 0; i < ArgTypes.size(); ++i) {
			if (ArgTypes[i]->isIntegerTy()) {
				GenericValue gv;
				gv.IntVal = APInt(64, 1); 
				arguments.push_back(gv);
			} else if (ArgTypes[i]->isDoubleTy()) {
				GenericValue gv;
				gv.DoubleVal = 0.0
				arguments.push_back(gv);
			} else if (ArgTypes[i]->isFloatTy()){
				GenericValue gv;
				gv.FloatVal = 0/*float*/;
				arguments.push_back(gv);
			}	
		}
		Builder.CreateCall(Callee, arguments)
		Builder.CreateRet(ConstantInt::get(Type::getInt32Ty(Ctx), 0))	
		std::error_code EC;
    raw_fd_ostream out("generated.ll", EC, sys::fs::F_None);
    NewModule->print(out, nullptr);
	}
}


PreservedAnalyses FPCorrPass::run(Function &F,
                                      FunctionAnalysisManager &AM) {
 	 
  return PreservedAnalyses::all();
}
