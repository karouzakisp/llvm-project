#ifndef LLVM_TRANSFORMS_UTILS_WIA_H
#define LLVM_TRANSFORMS_UTILS_WIA_H

#include "llvm/IR/PassManager.h"

namespace llvm {

class TargetMachine;

class WIAPass : public OptionalPassInfoMixin<WIAPass> {
  const TargetMachine *TM;
  // When true, process only minsize functions.
  // Keep it optional.
  bool OnlyMinSize;

public:
  explicit WIAPass(const TargetMachine *TM, bool OnlyMinSize = false)
      : TM(TM), OnlyMinSize(OnlyMinSize) {}

  LLVM_ABI PreservedAnalyses run(Function &F, FunctionAnalysisManager &AM);
};

LLVM_ABI bool isWIADisabled();

} // namespace llvm

#endif // LLVM_TRANSFORMS_UTILS_WIA_H
