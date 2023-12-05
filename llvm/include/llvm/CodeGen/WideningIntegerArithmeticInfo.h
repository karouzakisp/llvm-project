#ifndef LLVM_CODEGEN_WIDENINGINTEGERARITHMETICINFO_H
#define LLVM_CODEGEN_WIDENINGINTEGERARITHMETICINFO_H

#include <iostream>
#include "llvm/IR/Instruction.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/CodeGen/SelectionDAGNodes.h"
#include "llvm/Support/raw_ostream.h"

namespace llvm {


class Instruction;


  std::string OpcodesToStr[345] = {
      "DELETED_NODE",
      "EntryToken",
      "TokenFactor",
      "AssertSext",
      "AssertZext",
      "AssertAlign",
      "BasicBlock",
      "VALUETYPE",
      "CONDCODE",
      "Register",
      "RegisterMask",
      "Constant",
      "ConstantFP",
      "GlobalAddress",
      "GlobalTLSAddress",
      "FrameIndex",
      "JumpTable",
      "ConstantPool",
      "ExternalSymbol",
      "BlockAddress",
      "GLOBAL_OFFSET_TABLE",
      "FRAMEADDR",
      "RETURNADDR",
      "ADDROFRETURNADDR",
      "SPONENTRY",
      "LOCAL_RECOVER",
      "READ_REGISTER",
      "WRITE_REGISTER",
      "FRAME_TO_ARGS_OFFSET",
      "EH_DWARF_CFA",
      "EH_RETURN",
      "EH_SJLJ_SETJMP",
      "EH_SJLJ_LONGJMP",
      "EH_SJLJ_SETUP_DISPATCH",
      "TargetConstant",
      "TargetConstantFP",
      "TargetGlobalAddress",
      "TargetGlobalTLSAddress",
      "TargetFrameIndex",
      "TargetJumpTable",
      "TargetConstantPool",
      "TargetExternalSymbol",
      "TargetBlockAddress",
      "MCSymbol",
      "TargetIndex",
      "INTRINSIC_WO_CHAIN",
      "INTRINSIC_W_CHAIN",
      "INTRINSIC_VOID",
      "CopyToReg",
      "CopyFromReg",
      "UNDEF",
      "FREEZE",
      "EXTRACT_ELEMENT",
      "BUILD_PAIR",
      "MERGE_VALUES",
      "ADD",
      "SUB",
      "MUL",
      "SDIV",
      "UDIV",
      "SREM",
      "UREM",
      "SMUL_LOHI",
      "UMUL_LOHI",
      "SDIVREM",
      "UDIVREM",
      "CARRY_FALSE",
      "ADDC",
      "SUBC",
      "ADDE",
      "SUBE",
      "ADDCARRY",
      "SUBCARRY",
      "SADDO_CARRY",
      "SSUBO_CARRY",
      "SADDO",
      "UADDO",
      "SSUBO",
      "USUBO",
      "SMULO",
      "UMULO",
      "SADDSAT",
      "UADDSAT",
      "SSUBSAT",
      "USUBSAT",
      "SSHLSAT",
      "USHLSAT",
      "SMULFIX",
      "UMULFIX",
      "SMULFIXSAT",
      "UMULFIXSAT",
      "SDIVFIX",
      "UDIVFIX",
      "SDIVFIXSAT",
      "UDIVFIXSAT",
      "FADD",
      "FSUB",
      "FMUL",
      "FDIV",
      "FREM",
      "STRICT_FADD",
      "STRICT_FSUB",
      "STRICT_FMUL",
      "STRICT_FDIV",
      "STRICT_FREM",
      "STRICT_FMA",
      "STRICT_FSQRT",
      "STRICT_FPOW",
      "STRICT_FPOWI",
      "STRICT_FSIN",
      "STRICT_FCOS",
      "STRICT_FEXP",
      "STRICT_FEXP2",
      "STRICT_FLOG",
      "STRICT_FLOG10",
      "STRICT_FLOG2",
      "STRICT_FRINT",
      "STRICT_FNEARBYINT",
      "STRICT_FMAXNUM",
      "STRICT_FMINNUM",
      "STRICT_FCEIL",
      "STRICT_FFLOOR",
      "STRICT_FROUND",
      "STRICT_FROUNDEVEN",
      "STRICT_FTRUNC",
      "STRICT_LROUND",
      "STRICT_LLROUND",
      "STRICT_LRINT",
      "STRICT_LLRINT",
      "STRICT_FMAXIMUM",
      "STRICT_FMINIMUM",
      "STRICT_FP_TO_SINT",
      "STRICT_FP_TO_UINT",
      "STRICT_SINT_TO_FP",
      "STRICT_UINT_TO_FP",
      "STRICT_FP_ROUND",
      "STRICT_FP_EXTEND",
      "STRICT_FSETCC",
      "STRICT_FSETCCS",
      "FPTRUNC_ROUND",
      "FMA",
      "FMAD",
      "FCOPYSIGN",
      "FGETSIGN",
      "FCANONICALIZE",
      "IS_FPCLASS",
      "BUILD_VECTOR",
      "INSERT_VECTOR_ELT",
      "EXTRACT_VECTOR_ELT",
      "CONCAT_VECTORS",
      "INSERT_SUBVECTOR",
      "EXTRACT_SUBVECTOR",
      "VECTOR_REVERSE",
      "VECTOR_SHUFFLE",
      "VECTOR_SPLICE",
      "SCALAR_TO_VECTOR",
      "SPLAT_VECTOR",
      "SPLAT_VECTOR_PARTS",
      "STEP_VECTOR",
      "MULHU",
      "MULHS",
      "AVGFLOORS",
      "AVGFLOORU",
      "AVGCEILS",
      "AVGCEILU",
      "ABDS",
      "ABDU",
      "SMIN",
      "SMAX",
      "UMIN",
      "UMAX",
      "AND",
      "OR",
      "XOR",
      "ABS",
      "SHL",
      "SRA",
      "SRL",
      "ROTL",
      "ROTR",
      "FSHL",
      "FSHR",
      "BSWAP",
      "CTTZ",
      "CTLZ",
      "CTPOP",
      "BITREVERSE",
      "PARITY",
      "CTTZ_ZERO_UNDEF",
      "CTLZ_ZERO_UNDEF",
      "SELECT",
      "VSELECT",
      "SELECT_CC",
      "SETCC",
      "SETCCCARRY",
      "SHL_PARTS",
      "SRA_PARTS",
      "SRL_PARTS",
      "SIGN_EXTEND",
      "ZERO_EXTEND",
      "ANY_EXTEND",
      "TRUNCATE",
      "SINT_TO_FP",
      "UINT_TO_FP",
      "SIGN_EXTEND_INREG",
      "ANY_EXTEND_VECTOR_INREG",
      "SIGN_EXTEND_VECTOR_INREG",
      "ZERO_EXTEND_VECTOR_INREG",
      "FP_TO_SINT",
      "FP_TO_UINT",
      "FP_TO_SINT_SAT",
      "FP_TO_UINT_SAT",
      "FP_ROUND",
      "GET_ROUNDING",
      "SET_ROUNDING",
      "FP_EXTEND",
      "BITCAST",
      "ADDRSPACECAST",
      "FP16_TO_FP",
      "FP_TO_FP16",
      "STRICT_FP16_TO_FP",
      "STRICT_FP_TO_FP16",
      "BF16_TO_FP",
      "FP_TO_BF16",
      "FNEG",
      "FABS",
      "FSQRT",
      "FCBRT",
      "FSIN",
      "FCOS",
      "FPOWI",
      "FPOW",
      "FLOG",
      "FLOG2",
      "FLOG10",
      "FEXP",
      "FEXP2",
      "FCEIL",
      "FTRUNC",
      "FRINT",
      "FNEARBYINT",
      "FROUND",
      "FROUNDEVEN",
      "FFLOOR",
      "LROUND",
      "LLROUND",
      "LRINT",
      "LLRINT",
      "FMINNUM",
      "FMAXNUM",
      "FMINNUM_IEEE",
      "FMAXNUM_IEEE",
      "FMINIMUM",
      "FMAXIMUM",
      "FSINCOS",
      "LOAD",
      "STORE",
      "DYNAMIC_STACKALLOC",
      "BR",
      "BRIND",
      "BR_JT",
      "BRCOND",
      "BR_CC",
      "INLINEASM",
      "INLINEASM_BR",
      "EH_LABEL",
      "ANNOTATION_LABEL",
      "CATCHRET",
      "CLEANUPRET",
      "STACKSAVE",
      "STACKRESTORE",
      "CALLSEQ_START", // Beginning of a call sequence
      "CALLSEQ_END",   // End of a call sequence
      "VAARG",
      "VACOPY",
      "VAEND",
      "VASTART",
      "PREALLOCATED_SETUP",
      "PREALLOCATED_ARG",
      "SRCVALUE",
      "MDNODE_SDNODE",
      "PCMARKER",
      "READCYCLECOUNTER",
      "HANDLENODE",
      "INIT_TRAMPOLINE",
      "ADJUST_TRAMPOLINE",
      "TRAP",
      "DEBUGTRAP",
      "UBSANTRAP",
      "PREFETCH",
      "ARITH_FENCE",
      "MEMBARRIER",
      "ATOMIC_FENCE",
      "ATOMIC_LOAD",
      "ATOMIC_STORE",
      "ATOMIC_CMP_SWAP",
      "ATOMIC_CMP_SWAP_WITH_SUCCESS",
      "ATOMIC_SWAP",
      "ATOMIC_LOAD_ADD",
      "ATOMIC_LOAD_SUB",
      "ATOMIC_LOAD_AND",
      "ATOMIC_LOAD_CLR",
      "ATOMIC_LOAD_OR",
      "ATOMIC_LOAD_XOR",
      "ATOMIC_LOAD_NAND",
      "ATOMIC_LOAD_MIN",
      "ATOMIC_LOAD_MAX",
      "ATOMIC_LOAD_UMIN",
      "ATOMIC_LOAD_UMAX",
      "ATOMIC_LOAD_FADD",
      "ATOMIC_LOAD_FSUB",
      "ATOMIC_LOAD_FMAX",
      "ATOMIC_LOAD_FMIN",
      "ATOMIC_LOAD_UINC_WRAP",
      "ATOMIC_LOAD_UDEC_WRAP",
      "MLOAD",
      "MSTORE",
      "MGATHER",
      "MSCATTER",
      "LIFETIME_START",
      "LIFETIME_END",
      "GC_TRANSITION_START",
      "GC_TRANSITION_END",
      "GET_DYNAMIC_AREA_OFFSET",
      "PSEUDO_PROBE",
      "VSCALE",
      "VECREDUCE_SEQ_FADD",
      "VECREDUCE_SEQ_FMUL",
      "VECREDUCE_FADD",
      "VECREDUCE_FMUL",
      "VECREDUCE_FMAX",
      "VECREDUCE_FMIN",
      "VECREDUCE_ADD",
      "VECREDUCE_MUL",
      "VECREDUCE_AND",
      "VECREDUCE_OR",
      "VECREDUCE_XOR",
      "VECREDUCE_SMAX",
      "VECREDUCE_SMIN",
      "VECREDUCE_UMAX",
      "VECREDUCE_UMIN",
      "STACKMAP",
      "PATCHPOINT",
      "BUILTIN_OP_END"
  };



enum IntegerFillType{
  SIGN = 0,
  ZEROS,
  ANYTHING,
  UNDEFINED
};
  
enum WIAKind{
  WIAK_BINOP= 0,
  WIAK_UNOP,
  WIAK_FILL,
  WIAK_WIDEN,
  WIAK_WIDEN_GARBAGE,
  WIAK_NARROW,
  WIAK_DROP_EXT,
  WIAK_DROP_LOCOPY,
  WIAK_DROP_LOIGNORE,
  WIAK_EXTLO,
  WIAK_SUBSUME_FILL,
  WIAK_SUBSUME_INDEX,
  WIAK_NATURAL,
  WIAK_STORE,
  WIAK_CONSTANT,
  WIAK_LIT,
  WIAK_LOAD,
  WIAK_UNKNOWN
};

std::vector<std::string> WIAK_NAMES_VEC = {
    "WIAK_BINOP", "WIAK_UNOP", "WIAK_FILL", "WIAK_WIDEN", "WIAK_WIDEN_GARBAGE",
    "WIAK_NARROW", "WIAK_DROP_EXT", "WIAK_DROP_LOCOPY", "WIAK_DROP_LOIGNORE",
    "WIAK_EXTLO", "WIAK_SUBSUME_FILL", "WIAK_SUBSUME_INDEX", "WIAK_NATURAL",
    "WIAK_STORE", "WIAK_CONSTANT", "WIAK_LIT", "WIAK_LOAD", "WIAK_UNKNOWN"};



// This class is used to specify all the arguments that a solution
// of WideningIntegerArithmetic class requires.
class WideningIntegerSolutionInfo{

public:


protected:
  // We care only about integer arithmetic instructions
  unsigned       Opcode;
  // The new Opcode that has *this* as an operand 
  unsigned       NewOpcode;
  // Fill type for the upper bits of the expression
  // can be zero, one or garbage
  IntegerFillType   FillType; // τ
  unsigned char     FillTypeWidth; // n τ[n]
  // The current width of the instruction's destination operand
  unsigned char     Width;        // w
  // The updated width of the instruction's destination operand
  unsigned char     UpdatedWidth;
  // The cost of the solution
  short int         Cost;
  // Pointer to the SDNode that mapped to this solutionInfo
  SDNode            *Node;
private:  
  WIAKind Kind;
  // TODO can a SDNode have more than 4 Operands? 
  SmallPtrSet<WideningIntegerSolutionInfo *, 4> Operands;
public:

  WideningIntegerSolutionInfo() {}
  WideningIntegerSolutionInfo(WIAKind Kind_): Kind(Kind_) {} 
  WideningIntegerSolutionInfo(unsigned char Opcode_, IntegerFillType FillType_,
  unsigned FillTypeWidth_, unsigned char Width_, unsigned char UpdatedWidth_, 
    short int Cost_, WIAKind Kind_, SDNode* Node_): 
  Opcode(Opcode_),  FillType(FillType_), 
  FillTypeWidth(FillTypeWidth_), Width(Width_),
  UpdatedWidth(UpdatedWidth_),
  Cost(Cost_), Node(Node_), Kind(Kind_) {}
 
  WideningIntegerSolutionInfo(const WideningIntegerSolutionInfo &other)
  : Opcode(other.getOpcode()), FillType(other.getFillType()),
    FillTypeWidth(other.getFillTypeWidth()), Width(other.getWidth()),
    UpdatedWidth(other.getUpdatedWidth()), Cost(other.getCost()),
    Node(other.getNode()) , Kind(other.getKind()),
    Operands(other.getOperands()) {}
   

 

  unsigned getOpcode(void) const {
    return Opcode;

  }
  void setOpcode(unsigned Opcode_){
    Opcode = Opcode_;
  }


  bool operator==(const WideningIntegerSolutionInfo &o){
    return Kind == o.getKind() && Opcode == o.getOpcode() && 
           Cost == o.getCost() && Width == o.getWidth() &&
           UpdatedWidth == o.getUpdatedWidth();
  } 

  
  short int getCost(void) const {
    return Cost;
  }
  void setCost(short int Cost_){
    Cost = Cost_;
  }
  IntegerFillType getFillType(void) const {
    return FillType;
  }
  void setFillType(IntegerFillType FillType_){
    FillType = FillType_; 
  }

  unsigned getWidth(void) const {
    return Width;
  }
  void setWidth(unsigned char Width_){
    Width = Width_;
  }
  
  unsigned getUpdatedWidth(void) const {
    return UpdatedWidth;
  }

  void setUpdatedWidth(unsigned char UpdatedWidth_){
    UpdatedWidth = UpdatedWidth_;
  }
 
  void setKind(WIAKind Kind_){
    Kind = Kind_;
  } 
  WIAKind getKind() const {
    return Kind;
  }
  
  void setOperands(SmallPtrSet<WideningIntegerSolutionInfo *, 4>  Operands_){
    Operands = Operands_;
  }
  SmallPtrSet<WideningIntegerSolutionInfo *, 4>  getOperands(void) const {
    return std::move(Operands);
  }
  void addOperand(WideningIntegerSolutionInfo *Sol){
    Operands.insert(Sol);
  }

  WideningIntegerSolutionInfo* getOperand(short int i){
    return NULL;
  }
  
  unsigned  getFillTypeWidth(void) const {
    return FillTypeWidth;
  }

  void setFillTypeWidth(unsigned FillType_){
    FillTypeWidth = FillType_;
  }

  void setNode(SDNode *Node_){
    Node = Node_;
  }
  
  SDNode* getNode(void) const {
    return Node;
  }
 
  // Returns 0 if no one is redudant
  // Returns -1 if b is redundant given solution this
  // Returns 1 if "this" is redudant given solution b
  inline int IsRedudant(const WideningIntegerSolutionInfo &b){

    int n1 = FillTypeWidth;
    int c1 = Cost;
    int n2 = b.getFillTypeWidth();
    int c2 = b.getCost();
 
    if((*this) == b){
      return 0;
    }

    if(n1 >= n2  && c1 >= c2 )
      return 1;
    if(n2 >= n1 && c2 >= c1 )
      return -1;
   
    return 0; 
  }

}; // WideningIntegerSolutionInfo

  inline raw_ostream &operator<<(raw_ostream &out, 
                         WideningIntegerSolutionInfo const &Sol) {
    out << "\tOpcode: " << OpcodesToStr[Sol.getOpcode()] << '\n';
    out << "\tOpcode in number: " << Sol.getOpcode() << '\n';
    out << "\tWIA Kind : " << WIAK_NAMES_VEC[Sol.getKind()] << '\n';
    out << "\tFillType: " << Sol.getFillType() << '\n';
    out << "\tFillTypeWidth: " << Sol.getFillTypeWidth() << '\n';
    out << "\tOldWidth: " << Sol.getWidth() << '\n';
    out << "\tUpdatedWidth: " << Sol.getUpdatedWidth() << '\n';
    out << "\tCost : "<< Sol.getCost() << '\n';
    return out;
  } 

class WIA_BINOP : public WideningIntegerSolutionInfo
{
  public:
  ~WIA_BINOP() {}
  WIA_BINOP() {}
  WIA_BINOP(unsigned char Opcode_, IntegerFillType FillType_,
            unsigned char FillTypeWidth_,
            unsigned char Width_, unsigned char UpdatedWidth_, 
            short int Cost_, SDNode *Node_): 
      WideningIntegerSolutionInfo::WideningIntegerSolutionInfo(
        Opcode_, FillType_, FillTypeWidth_, Width_, 
        UpdatedWidth_, Cost_, WIAK_BINOP, Node_) {}
  
  static inline bool classof(WIA_BINOP const *) { return true; }
  static inline bool classof(WideningIntegerSolutionInfo const *Base){
    switch(Base->getKind()){
      case WIAK_BINOP: return true;
      default: return false;
    } 
  } 
  
};


class WIA_FILL : public WideningIntegerSolutionInfo
{
  public:
  ~WIA_FILL() {};
  WIA_FILL() {;};
  WIA_FILL(unsigned char Opcode_, IntegerFillType FillType_,
            unsigned char FillTypeWidth_,
            unsigned char Width_, unsigned char UpdatedWidth_, 
            short int Cost_, SDNode *Node_): 
      WideningIntegerSolutionInfo::WideningIntegerSolutionInfo(
        Opcode_, FillType_, FillTypeWidth_, Width_, 
        UpdatedWidth_, Cost_, WIAK_FILL, Node_) {} 

  static inline bool classof(WIA_FILL const *) { return true; }
  static inline bool classof(WideningIntegerSolutionInfo const *Base){
    switch(Base->getKind()){
      case WIAK_FILL: return true;
      default: return false;
    } 
  }
};


class WIA_WIDEN : public WideningIntegerSolutionInfo
{
  public:
  WIA_WIDEN() {}
  ~WIA_WIDEN() {}
  WIA_WIDEN(unsigned char Opcode_, IntegerFillType FillType_,
            unsigned char FillTypeWidth_,
            unsigned char Width_, unsigned char UpdatedWidth_, 
            short int Cost_, SDNode *Node_): 
      WideningIntegerSolutionInfo::WideningIntegerSolutionInfo(
        Opcode_, FillType_, FillTypeWidth_, Width_, 
        UpdatedWidth_, Cost_, WIAK_WIDEN, Node_) {}
  
  static inline bool classof(WIA_WIDEN const *) { return true; }
  static inline bool classof(WideningIntegerSolutionInfo const *Base){
    switch(Base->getKind()){
      case WIAK_WIDEN: return true;
      default: return false;
    } 
  }
};

class WIA_WIDEN_GARBAGE : public WideningIntegerSolutionInfo
{
  public:
  WIA_WIDEN_GARBAGE() {}
  ~WIA_WIDEN_GARBAGE() {}
  WIA_WIDEN_GARBAGE(unsigned char Opcode_, IntegerFillType FillType_,
            unsigned char FillTypeWidth_,
            unsigned char Width_, unsigned char UpdatedWidth_, 
            short int Cost_, SDNode *Node_): 
      WideningIntegerSolutionInfo::WideningIntegerSolutionInfo(
        Opcode_, FillType_, FillTypeWidth_, Width_, 
        UpdatedWidth_, Cost_, WIAK_WIDEN_GARBAGE, Node_ ) {}

  static inline bool classof(WIA_WIDEN_GARBAGE const *) { return true; }
  static inline bool classof(WideningIntegerSolutionInfo const *Base){
    switch(Base->getKind()){
      case WIAK_WIDEN_GARBAGE: return true;
      default: return false;
    } 
  }
  
};

class WIA_NARROW : public WideningIntegerSolutionInfo
{
  public:
  WIA_NARROW() {}
  ~WIA_NARROW() {}
  WIA_NARROW(unsigned char Opcode_, IntegerFillType FillType_,
            unsigned char FillTypeWidth_,
            unsigned char Width_, unsigned char UpdatedWidth_, 
            short int Cost_, SDNode *Node_): 
      WideningIntegerSolutionInfo::WideningIntegerSolutionInfo(
        Opcode_, FillType_ , FillTypeWidth_ , Width_, 
        UpdatedWidth_, Cost_, WIAK_NARROW, Node_){}

  static inline bool classof(WIA_NARROW const *) { return true; }
  static inline bool classof(WideningIntegerSolutionInfo const *Base){
    switch(Base->getKind()){
      case WIAK_NARROW: return true;
      default: return false;
    } 
  }
  
};


class WIA_DROP_EXT : public WideningIntegerSolutionInfo
{
  public:
  WIA_DROP_EXT() {}
  ~WIA_DROP_EXT() {}
  WIA_DROP_EXT(unsigned char Opcode_, IntegerFillType FillType_,
            unsigned char FillTypeWidth_,
            unsigned char Width_, unsigned char UpdatedWidth_, 
            short int Cost_, SDNode *Node_): 
      WideningIntegerSolutionInfo::WideningIntegerSolutionInfo(
        Opcode_, FillType_, FillTypeWidth_, Width_, 
        UpdatedWidth_, Cost_, WIAK_DROP_EXT, Node_) {}

  static inline bool classof(WIA_DROP_EXT const *) { return true; }
  static inline bool classof(WideningIntegerSolutionInfo const *Base){
    switch(Base->getKind()){
      case WIAK_DROP_EXT: return true;
      default: return false;
    } 
  }
  
};


class WIA_DROP_LOCOPY : public WideningIntegerSolutionInfo
{
  public:
  WIA_DROP_LOCOPY() {}
  ~WIA_DROP_LOCOPY() {}
  WIA_DROP_LOCOPY(unsigned char Opcode_, IntegerFillType FillType_,
            unsigned char FillTypeWidth_, 
            unsigned char Width_, unsigned char UpdatedWidth_, 
            short int Cost_, SDNode *Node_): 
      WideningIntegerSolutionInfo::WideningIntegerSolutionInfo(
        Opcode_, FillType_, FillTypeWidth_, Width_,
        UpdatedWidth_, Cost_, WIAK_DROP_LOCOPY, Node_) {}

  static inline bool classof(WIA_DROP_LOCOPY const *) { return true; }
  static inline bool classof(WideningIntegerSolutionInfo const *Base){
    switch(Base->getKind()){
      case WIAK_DROP_LOCOPY: return true;
      default: return false;
    } 
  }
  
};


class WIA_DROP_LOIGNORE : public WideningIntegerSolutionInfo
{
  public:
  WIA_DROP_LOIGNORE() {}
  ~WIA_DROP_LOIGNORE() {}
  WIA_DROP_LOIGNORE(unsigned char Opcode_, IntegerFillType FillType_,
            unsigned char FillTypeWidth_,
            unsigned char Width_, unsigned char UpdatedWidth_, 
            short int Cost_, SDNode *Node_): 
      WideningIntegerSolutionInfo::WideningIntegerSolutionInfo(
        Opcode_, FillType_, FillTypeWidth_, Width_, 
        UpdatedWidth_, Cost_, WIAK_DROP_LOIGNORE,Node_) {}

  static inline bool classof(WIA_DROP_LOIGNORE const *) { return true; }
  static inline bool classof(WideningIntegerSolutionInfo const *Base){
    switch(Base->getKind()){
      case WIAK_DROP_LOIGNORE: return true;
      default: return false;
    } 
  }
  
};


class WIA_EXTLO : public WideningIntegerSolutionInfo
{
  public:
  WIA_EXTLO() {}
  ~WIA_EXTLO() {}
  WIA_EXTLO(unsigned char Opcode_, IntegerFillType FillType_,
            unsigned char FillTypeWidth_,
            unsigned char Width_, unsigned char UpdatedWidth_, 
            short int Cost_, SDNode *Node_): 
      WideningIntegerSolutionInfo::WideningIntegerSolutionInfo(
        Opcode_, FillType_, FillTypeWidth_, Width_, 
        UpdatedWidth_, Cost_, WIAK_EXTLO, Node_) {}

  static inline bool classof(WIA_EXTLO const *) { return true; }
  static inline bool classof(WideningIntegerSolutionInfo const *Base){
    switch(Base->getKind()){
      case WIAK_EXTLO: return true;
      default: return false;
    } 
  }
  
};


class WIA_SUBSUME_FILL : public WideningIntegerSolutionInfo
{
  public:
  WIA_SUBSUME_FILL() {}
  ~WIA_SUBSUME_FILL() {}
  WIA_SUBSUME_FILL(unsigned char Opcode_, IntegerFillType FillType_,
            unsigned char FillTypeWidth_,
            unsigned char Width_, unsigned char UpdatedWidth_, 
            short int Cost_, SDNode *Node_): 
      WideningIntegerSolutionInfo::WideningIntegerSolutionInfo(
        Opcode_, FillType_, FillTypeWidth_, Width_, 
        UpdatedWidth_, Cost_, WIAK_SUBSUME_FILL, Node_) {}

  static inline bool classof(WIA_SUBSUME_FILL const *) { return true; }
  static inline bool classof(WideningIntegerSolutionInfo const *Base){
    switch(Base->getKind()){
      case WIAK_SUBSUME_FILL: return true;
      default: return false;
    } 
  }
  
};


class WIA_SUBSUME_INDEX : public WideningIntegerSolutionInfo
{
  public:
  WIA_SUBSUME_INDEX() {}
  ~WIA_SUBSUME_INDEX() {}
  WIA_SUBSUME_INDEX(unsigned char Opcode_, IntegerFillType FillType_,
            unsigned char FillTypeWidth_,
            unsigned char Width_, unsigned char UpdatedWidth_, 
            short int Cost_, SDNode *Node_): 
      WideningIntegerSolutionInfo::WideningIntegerSolutionInfo(
        Opcode_, FillType_, FillTypeWidth_, Width_, 
        UpdatedWidth_, Cost_, WIAK_SUBSUME_INDEX, Node_) {}

  static inline bool classof(WIA_SUBSUME_INDEX const *) { return true; }
  static inline bool classof(WideningIntegerSolutionInfo const *Base){
    switch(Base->getKind()){
      case WIAK_SUBSUME_INDEX: return true;
      default: return false;
    } 
  }
  
};


class WIA_NATURAL : public WideningIntegerSolutionInfo
{
  public:
  WIA_NATURAL() {}
  ~WIA_NATURAL() {}
  WIA_NATURAL(unsigned char Opcode_, IntegerFillType FillType_,
            unsigned char FillTypeWidth_,
            unsigned char Width_, unsigned char UpdatedWidth_, 
            short int Cost_, SDNode *Node_): 
      WideningIntegerSolutionInfo::WideningIntegerSolutionInfo(
        Opcode_, FillType_, FillTypeWidth_, Width_, 
        UpdatedWidth_, Cost_, WIAK_NATURAL, Node_) {}

  static inline bool classof(WIA_NATURAL const *) { return true; }
  static inline bool classof(WideningIntegerSolutionInfo const *Base){

    switch(Base->getKind()){
      case WIAK_NATURAL: return true;
      default: return false;

    } 
  }
  
};

class WIA_LOAD : public WideningIntegerSolutionInfo
{
  public:
  WIA_LOAD() {}
  ~WIA_LOAD() {}
  WIA_LOAD(unsigned char Opcode_, IntegerFillType FillType_,
            unsigned char FillTypeWidth_,
            unsigned char Width_, unsigned char UpdatedWidth_, 
            short int Cost_, SDNode *Node_): 
      WideningIntegerSolutionInfo::WideningIntegerSolutionInfo(
        Opcode_, FillType_, FillTypeWidth_, Width_, 
        UpdatedWidth_, Cost_, WIAK_LOAD, Node_) {}

  static inline bool classof(WIA_LOAD const *) { return true; }
  static inline bool classof(WideningIntegerSolutionInfo const *Base){
    switch(Base->getKind()){
      case WIAK_LOAD: return true;
      default: return false;
    } 
  }
  
};

class WIA_STORE : public WideningIntegerSolutionInfo
{
  public:
  WIA_STORE() {}
  ~WIA_STORE() {}
  WIA_STORE(unsigned char Opcode_, IntegerFillType FillType_,
            unsigned char FillTypeWidth_,
            unsigned char Width_, unsigned char UpdatedWidth_, 
            short int Cost_, SDNode *Node_): 
      WideningIntegerSolutionInfo::WideningIntegerSolutionInfo(
        Opcode_, FillType_, FillTypeWidth_, Width_, 
        UpdatedWidth_, Cost_, WIAK_STORE, Node_) {}

  static inline bool classof(WIA_STORE const *) { return true; }
  static inline bool classof(WideningIntegerSolutionInfo const *Base){
    switch(Base->getKind()){
      case WIAK_STORE: return true;
      default: return false;
    } 
  }
  
};

class WIA_UNOP : public WideningIntegerSolutionInfo
{
  public:
  WIA_UNOP() {}
  ~WIA_UNOP() {}
  WIA_UNOP(unsigned char Opcode_, IntegerFillType FillType_,
            unsigned char FillTypeWidth_,
            unsigned char Width_, unsigned char UpdatedWidth_, 
            short int Cost_, SDNode *Node_): 
      WideningIntegerSolutionInfo::WideningIntegerSolutionInfo(
        Opcode_, FillType_, FillTypeWidth_, Width_, 
        UpdatedWidth_, Cost_, WIAK_UNOP, Node_) {}

  static inline bool classof(WIA_UNOP const *) { return true; }
  static inline bool classof(WideningIntegerSolutionInfo const *Base){
    switch(Base->getKind()){
      case WIAK_UNOP: return true;
      default: return false;
    } 
  }
  
};

class WIA_CONSTANT : public WideningIntegerSolutionInfo
{
  public:
  WIA_CONSTANT() {}
  ~WIA_CONSTANT() {}
  WIA_CONSTANT(unsigned char Opcode_, IntegerFillType FillType_,
            unsigned char FillTypeWidth_,
            unsigned char Width_, unsigned char UpdatedWidth_, 
            short int Cost_, SDNode *Node_): 
      WideningIntegerSolutionInfo::WideningIntegerSolutionInfo(
        Opcode_, FillType_, FillTypeWidth_, Width_, 
        UpdatedWidth_, Cost_, WIAK_CONSTANT, Node_) {}

  static inline bool classof(WIA_CONSTANT const *) { return true; }
  static inline bool classof(WideningIntegerSolutionInfo const *Base){
    switch(Base->getKind()){
      case WIAK_CONSTANT: return true;
      default: return false;
    } 
  }
  
};







} // namespace llvm;


#endif // LLVM_CODEGEN_WIDENINGINTEGERARITHMETIC_H
