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
#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/GenericValue.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/GenericValue.h"
#include "llvm/ExecutionEngine/Interpreter.h"
#include "llvm/ExecutionEngine/JITEventListener.h"
#include "llvm/ExecutionEngine/SectionMemoryManager.h"


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

double executeFPFunction(Function &F){
	Function *VisitedF = &F;

	Type *RetTy = VisitedF->getReturnType();
	assert(RetTy->isFloatingPointTy() && "Function does not return Floating Point Type!");	
	std::vector<Type *> ArgTypes;
	std::vector<StringRef> ArgNames;
	LLVMContext &Ctx = VisitedF->getContext();

	FunctionType *FTy = VisitedF->getFunctionType();

	for (Argument &arg : VisitedF->args()) {
		ArgTypes.push_back(arg.getType());
		ArgNames.push_back(arg.getName());
	}

	FunctionType *funcPointerType = FunctionType::get(VisitedF->getReturnType(), ArgTypes, false);
	
	std::unique_ptr<Module> module = std::make_unique<Module>("MyModule", Ctx);
	module->getOrInsertFunction(VisitedF->getName(), funcPointerType);


	// Create an execution engine with a JIT compiler
	EngineBuilder builder(std::move(module));
	builder.setEngineKind(EngineKind::Interpreter);
  ExecutionEngine *EE = builder.create();

	void *RFP = EE->getPointerToFunction(VisitedF);
	auto *GeneratedFunction = reinterpret_cast<GenericValue (*)(ArrayRef<GenericValue> In)>(RFP);



	SmallVector<GenericValue, 4> arguments;
	for (size_t i = 0; i < ArgTypes.size(); ++i) {
		if (ArgTypes[i]->isIntegerTy()) {
			GenericValue gv;
			gv.IntVal = APInt(64, 1); // Replace with appropriate integer value
			arguments.push_back(gv);
		} else if (ArgTypes[i]->isDoubleTy()) {
			GenericValue gv;
			gv.DoubleVal = 0.0/*double_value*/; // Replace with appropriate double value
			arguments.push_back(gv);
		} else if (ArgTypes[i]->isFloatTy()){
			GenericValue gv;
			gv.FloatVal = 0/*float*/;
			arguments.push_back(gv);
		}	
	}
	if (GeneratedFunction) {
		GenericValue returnValue = GeneratedFunction(arguments);
		if(RetTy->isFloatTy()){
			float fval = returnValue.FloatVal;
		}else if(RetTy->isDoubleTy()){
			double fval = returnValue.DoubleVal;
		}else{

		}
	}else{
		dbgs() << "Error here .. " << '\n';
	}
	return 0;
}


PreservedAnalyses FPCorrPass::run(Function &F,
                                      FunctionAnalysisManager &AM) {
 	 
  return PreservedAnalyses::all();
}
