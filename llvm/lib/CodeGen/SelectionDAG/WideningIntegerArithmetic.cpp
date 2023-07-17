///===- WideningIntegerArithmetic ------------------------------------------===
//
//



#include "llvm/CodeGen/SelectionDAG.h"
#include "llvm/CodeGen/TargetLowering.h"
#include "llvm/CodeGen/SelectionDAGNodes.h"
#include "llvm/CodeGen/WideningIntegerArithmeticInfo.h"

using namespace llvm;

class WideningIntegerArithmetic {
  SelectionDAG &DAG;
  const TargetLowering &TLI; 

  public:
    WideningIntegerArithmetic(SelectionDAG &D):
      DAG(D), TLI(D.getTargetLoweringInfo()) {} 
  
    void solve();

  private:
    DenseMap<int, WideningIntegerSolutionInfo> SolutionMap;
    void visit(SDNode *Node);
};


void WideningIntegerArithmetic::visit(SDNode *Node){
  int NumOperands = Node->getNumOperands();
  for (const SDValue &value : Node->op_values() ){
    SDNode *VisitedNode = value.getNode();
  }
}

void WideningIntegerArithmetic::solve(){
  
  for (SDNode &Node : DAG.allnodes()){
      // if not found visit it
      visit(&Node);  
    
  } 
} 
