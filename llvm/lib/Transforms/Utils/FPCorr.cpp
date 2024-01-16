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
#include "llvm/IR/IRBuilder.h"
#include "llvm/Transforms/Utils/FPCorr.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/Twine.h"
#include "llvm/ExecutionEngine/GenericValue.h"
#include "llvm/ExecutionEngine/Orc/LLJIT.h"
#include "llvm/ExecutionEngine/Orc/ThreadSafeModule.h"
#include "llvm/ExecutionEngine/Orc/ObjectLinkingLayer.h"

#include "llvm/Support/CommandLine.h"
#include "llvm/Support/InitLLVM.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/Error.h"
#include "llvm/Pass.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include <vector>
#include <mpfr.h>
#include <cassert>
#include <random>



using namespace llvm;
using namespace llvm::orc;


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
        errs() << "Visited " << Operand->getOpcode() << "\n";
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
		if (auto *GEP = dyn_cast<GetElementPtrInst>(I)) {
			if (auto *GV = dyn_cast<GlobalVariable>(GEP->getPointerOperand())) {
				return true;			
			}
		}
	}
	return false;	
}

bool canExecuteF(Function &F){
	Type *RetTy = F.getReturnType();
  if(!RetTy->isFloatingPointTy()){
    return false;
  }
	if(hasGlobalVariables(F)){
		return false;
	}
  return true;
}

double executeFPFunction(Function &F){

  std::random_device rd;
  std::default_random_engine generator(rd());
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

	FunctionType *Fty = VisitedF->getFunctionType();	
	FunctionCallee Callee = OldModule->getOrInsertFunction(VisitedF->getName(), Fty);
	
	
	std::unique_ptr<Module> NewModule = std::make_unique<Module>("MyNewModule", Ctx);
	std::error_code EC;
  raw_fd_ostream out("generated.ll", EC);
  F.print(out);
  
  std::uniform_real_distribution<float> f_distribution(-10.0, 10.0);
  std::uniform_real_distribution<float> r_distribution(1000, 1000);
  out << "declare float @" << F.getName() << "("; 
  for (size_t i = 0; i < ArgTypes.size(); ++i) {
    if(i > 0 ) out << ", ";

    if (ArgTypes[i]->isIntegerTy()) {
      out << "i32";      
    } else if (ArgTypes[i]->isDoubleTy()) {
      out << "f64";
    } else if (ArgTypes[i]->isFloatTy()){
      out << "f32";
    }	
  }
  out << ")\n";
  out << "define float @CallerFunction() {\n";

  // TODO change variables to match the F variables types
  out << " %result = call float @" << F.getName() << "(";
  for (size_t i = 0; i < ArgTypes.size(); ++i) {
    if(i > 0 ) out << ", ";

    if (ArgTypes[i]->isIntegerTy()) {
      out << "i32 " + std::to_string(r_distribution(generator)); 
    } else if (ArgTypes[i]->isDoubleTy()) {
      out << "f64 " + std::to_string(f_distribution(generator)); 
    } else if (ArgTypes[i]->isFloatTy()){
      out << "f32 " + std::to_string(f_distribution(generator));
    }
  }
  errs() << "writing to out !!! " << "\n";
  out << ")\n";
  out << " ret float %result \n";
  out << "}\n";
  return 0.0;
}

float tryToJitFunction(Function &F){
	Function *VisitedF = &F;
	LLVMContext &Ctx = VisitedF->getContext();
  auto Context = std::make_unique<LLVMContext>();
  auto NewM = std::make_unique<Module>("fpcorrM", Ctx);
  auto Twn = Twine(VisitedF->getName());
  Function *ClonedF = Function::Create(F.getFunctionType(), F.getLinkage(),
                                      Twn, std::move(NewM.get()));
  auto J = LLJITBuilder().create();
//  auto M = ThreadSafeModule(std::move(NewM), std::move(Context));
  
  return 0.0;
}


PreservedAnalyses FPCorrPass::run(Function &F,
                                      FunctionAnalysisManager &AM) {
  dbgs() << "I am here !" << "\n";
 	if(canExecuteF(F)){
  //    executeFPFunction(F);
  } 
  return PreservedAnalyses::all();
}


llvm::PassPluginLibraryInfo getFpcorrPluginInfo() {
  return {LLVM_PLUGIN_API_VERSION, "Fpcorr", LLVM_VERSION_STRING,
          [](PassBuilder &PB) {
            PB.registerVectorizerStartEPCallback(
                [](llvm::FunctionPassManager &PM, OptimizationLevel Level) {
                  PM.addPass(FPCorrPass());
                });
            PB.registerPipelineParsingCallback(
                [](StringRef Name, llvm::FunctionPassManager &PM,
                   ArrayRef<llvm::PassBuilder::PipelineElement>) {
                  if (Name == "Fpcorr") {
                    PM.addPass(FPCorrPass());
                    return true;
                  }
                  return false;
                });
          }};
}


#ifndef LLVM_FPCORR_LINK_INTO_TOOLS
extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
  return getFpcorrPluginInfo();
}
#endif
