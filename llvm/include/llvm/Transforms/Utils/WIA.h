#ifndef LLVM_TRANSFORMS_UTILS_WIA_H
#define LLVM_TRANSFORMS_UTILS_WIA_H

#include "llvm/IR/PassManager.h"

namespace llvm {



class WIAPass : public PassInfoMixin<WIAPass> {
public:
  PreservedAnalyses run(Function &F, FunctionAnalysisManager &AM);
};


} // namespace llvm

#endif // LLVM_TRANSFORMS_UTILS_WIA_H
