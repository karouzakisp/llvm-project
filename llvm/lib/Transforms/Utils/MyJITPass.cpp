//===---------------------------- MyJITPass.cpp ---------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "llvm/Transforms/Utils/MyJITPass.h"
#include "llvm/ExecutionEngine/Orc/LLJIT.h"

#include <expected>

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

auto tryToJitFunction(Function &F) -> std::expected<double, jit_error>
{
  auto J = LLJITBuilder().create();
  if(!J) {
    errs() << "FPCorrPass could not create JIT instance for "
      << F.getName() << ": " << toString(J.takeError()) << "\n";
    return std::unexpected(jit_error::jit_instance);
  }
  auto FM = cloneToNewContext(*F.getParent(),
              [&](const GlobalValue &GV) { return &GV == &F; });

  if cloneToNewContext( auto Err = (*J)->addIRModule(std::move(FM)) ){
    errs() << "MyJITPass could not add extracted module for "
      << F.getName() << ": " << toString(std::move(Err)) << "\n";
    return std::unexpected(jit_error::add_module);
  }
  
  auto FSym = (*J)->lookup(F.getName());
  if (!FSym) {
    errs() << "FPCorrPass could not get JIT'd symbol for "
      << F.getName() << ": " << toString(FSym.takeError()) << "\n";
    return std::unexpected(jit_error::lookup);
  }
  auto *JittedF = FSym->toPtr<float(float)>();
  auto Res =  JittedF(41.1);
  errs() << "Result: " << Res << "\n";
  return Res;
}



PreservedAnalyses MyJITPass::run(Function &F,
				 FunctionAnalysisManager &AM) {
  if(canExecuteF(F)){
    float y = tryToJitFunction(F);
  }
}
