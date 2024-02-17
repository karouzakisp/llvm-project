#include "llvm/Transforms/IPO/WideningIntegerArithmetic.h"


using namespace llvm;


PreservedAnalyses WIAPass::run(Module &M,
															 ModuleAnalysisManager &MAM){
	errs() << M.getName() << "\n";
	return PreservedAnalyses::all();

}
