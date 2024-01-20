//===---------------------------- MyJITPass.cpp ---------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "llvm/Transforms/Utils/MyJITPass.h"
#include "llvm/ExecutionEngine/Orc/LLJIT.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Value.h"
#include "llvm/IR/Instructions.h"

#include <optional>
#include <vector>


using namespace llvm;
using namespace llvm::orc;

enum class jit_error
{
  jit_instance,
  add_module,
  lookup_symbol,
};

bool canExecuteF(Function &F){
  Type *RetTy = F.getReturnType();
  if(!RetTy->isFloatingPointTy()){
    return false;
  }
  return true;
}

std::vector<Value*> createArgValues(Module *M, std::vector<Type *>ArgTypes,
        IRBuilder<> &Builder){
  std::vector<Value *> ArgValues;
  for(Type* Arg : ArgTypes){
    if(auto *Iarg = dyn_cast<IntegerType>(Arg) ){
      switch(Iarg->getBitWidth()){
        case 8:   ArgValues.push_back(Builder.getInt8(5));
                  break;
        case 16:  ArgValues.push_back(Builder.getInt16(20));
                  break;
        case 32:  ArgValues.push_back(Builder.getInt32(22));
                  break;
        case 64:  ArgValues.push_back(Builder.getInt64(62));
                  break;
      }
    }else if(Arg->isFloatTy() ){

    }else if(Arg->isHalfTy() ){

    }else if(Arg->isBFloatTy() ){
      
    }else if(Arg->isDoubleTy() ){

    }else if(Arg->isX86_FP80Ty()){

    }else if(Arg->isFP128Ty()){

    }else if(Arg->isPPC_FP128Ty()){

    }
  }
  return ArgValues;

}


std::optional<double> tryToJitFunction(Function &F)
{
  auto J = LLJITBuilder().create();
  if(!J) {
    errs() << "FPCorrPass could not create JIT instance for "
      << F.getName() << ": " << toString(J.takeError()) << "\n";
    return std::nullopt;
  }
  std::vector<Type *> ArgTypes;
  for (Argument &arg : F.args()) {
    ArgTypes.push_back(arg.getType());
  }

  
  auto FM = cloneToNewContext(*F.getParent(),
              [&](const GlobalValue &GV) { return &GV == &F; });

  Module *M = FM.getModuleUnlocked();
  LLVMContext &Ctx = M->getContext();
  FunctionType *FTy=  FunctionType::get(Type::getDoubleTy(Ctx), ArgTypes, 
                                                                false); 
  Function *WF = Function::Create(FTy, Function::ExternalLinkage, 
      "Wrapper"+F.getName(), M);
  BasicBlock *BB = BasicBlock::Create(Ctx, "entry", WF);
  IRBuilder<> Builder(BB);
  
  if (auto Err = (*J)->addIRModule(std::move(FM))){
    errs() << "MyJITPass could not add extracted module for "
      << WF->getName() << ": " << toString(std::move(Err)) << "\n";
    return std::nullopt;
  }

  auto FSym = (*J)->lookup(WF->getName());
  if (!FSym) {
    errs() << "FPCorrPass could not get JIT'd symbol for "
      << WF->getName() << ": " << toString(FSym.takeError()) << "\n";
    return std::nullopt;
  }
  std::vector<Value*> ArgValues = createArgValues(M, ArgTypes, Builder); 
  FunctionCallee Callee = M->getOrInsertFunction(F.getName(), F.getFunctionType());
  Value *CallRes = Builder.CreateCall(Callee, ArgValues);
  ReturnInst *RetInst = Builder.CreateRet(CallRes);
  auto Res =  JittedF(41.1);
  errs() << "Result: " << Res << "\n";
  return Res;
}



PreservedAnalyses MyJITPass::run(Function &F,
				 FunctionAnalysisManager &AM) {
  if(canExecuteF(F)){
    std::optional<double> y = tryToJitFunction(F);
  }
}
