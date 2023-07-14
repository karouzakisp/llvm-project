#ifndef LLVM_ANALYSIS_WIDENINGINTEGERARITHMETICINFO_H
#define LLVM_ANALYSIS_WIDENINGINTEGERARITHMETICINFO_H


#include "llvm/IR/Instruction.h"

namespace llvm {


class Instruction;

enum IntegerFillType{
  sign_bits = 0,
  zeroes,
  anything 
};


// This class is used to specify all the arguments that a solution
// of WideningIntegerArithmetic class requires.
class WideningIntegerSolutionInfo{
public:
 
  ~WideningIntegerInfo();
  WideningIntegerInfo() = default; 
  WideningIntegerInfo(Instruction *Instr, IntegerFillType fillType_,
                      unsigned char width_)
    : Instruction(Instr), fillType(fillType_), width(width_){} 
  WideningIntegerInfo(WideningIntegerInfo &&Arg)
    : instruction(Arg.instruction), 
      fillType(Arg.fillType), width(Arg.width), {} 
  

private:
  WideningIntegerInfo(const WideningIntegerInfo&) = delete
  void operator=(const WideningIntegerInfo&) = delete
  

  // We care only about integer arithmetic instructions
  Instruction       *IntegerInstr=nullptr;

  // The updated instruction if we add an extension
  Instruction       *UpdatedInstruction=nullptr;

  // The cost of the extension 
  short int         ExtensionCost=0;

  // Fill type for the upper bits of the register
  // can be zero, one or garbage
  IntegerFillType   FillType=anything;

  // The updated width of the instruction's destination operand
  unsigned char     UpdatedWidth=0;

  // The current width of the instruction's destination operand
  unsigned char     Width=0;

public:
  Instruction *getInstruction(void){
    return IntegerInstr;
  }
  void setIntegerInstruction(Instruction *IntegerInstr_){
    IntegerInstr = IntegerInstr_;
  }  

  Instruction *getUpdatedInstruction(void);
    return UpdatedInstruction;
  }
  void setUpdatedInstruction(Instruction *UpdatedInstr){
    UpdatedInstruction = UpdatedInstr;
  }
  
  short int getExtensionCost(void){
    return ExtensionCost;
  }
  void setExtensionCost(short int ExtensionCost_){
    ExtensionCost = ExtensionCost_
  }
  Integer_fill_type getIntegerFillType(void){
    return FillType;
  }
  void setIntegerFillType(IntegerFillType FillType_){
    FillType = FillType_; 
  }

  unsigned char getWidth(void){
    return Width;
  }
  void setWidth(unsigned char Width_){
    Width = Width_;
  }
  
  unsigned char getUpdatedWidth(void){
    return UpdatedWidth;
  }

  void setUpdatedWidth(unsigned char UpdatedWidth_){
    UpdatedWidth = UpdatedWidth_;
  }

  

}; // WideningIntegerSolutionInfo




} // namespace llvm;


#endif // LLVM_ANALYSIS_UTILS_WIDENINGINTEGERARITHMETIC_H
