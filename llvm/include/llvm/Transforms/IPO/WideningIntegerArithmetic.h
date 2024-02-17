#ifndef LLVM_WIDENING_INTEGER_ARITHMETIC_H
#define LLVM_WIDENING_INTEGER_ARITHMETIC_H

#include "llvm/IR/PassManager.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/CodeGen/SelectionDAG.h"
#include "llvm/CodeGen/TargetLowering.h"
#include "llvm/CodeGen/SelectionDAGNodes.h"
#include "llvm/CodeGen/WideningIntegerArithmeticInfo.h"
#include "llvm/CodeGen/ValueTypes.h"
#include "llvm/ADT/SmallSet.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Transforms/IPO/WideningIntegerArithmetic.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/TargetParser/Triple.h"
#include "llvm/Support/TypeSize.h"
#include "llvm/Support/CodeGen.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"



namespace llvm {

class WIAPass : public PassInfoMixin<WIAPass>{
		

	public:
		PreservedAnalyses run(Module &M, ModuleAnalysisManager &MAM);
		WIAPass(){}	
};



}

#endif
