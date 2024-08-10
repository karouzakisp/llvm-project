#ifndef LLVM_CODEGEN_WIDENINGINTEGERARITHMETICINFO_H
#define LLVM_CODEGEN_WIDENINGINTEGERARITHMETICINFO_H

#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/IR/Value.h"
#include "llvm/Support/raw_ostream.h"
#include <iostream>
#include <queue>

namespace llvm {

#define FILL_INST_OPC 66
#define CONSTANT_INT_OPC 67
#define UNKNOWN_OPC 68

class Value;

std::string OpcodesToStr[70] = {
    "Empty Instr",
    "Return",
    "BranchInst",
    "SwitchInst",
    "IndirectBranch",
    "InvokeInst",
    "ResumeInst",
    "UnreachableInst",
    "CleanupReturnInst",
    "CatchReturnInst",
    "CatchSwitchInst",
    "CallBrInst", // A call-site terminator ",
    "UnaryOperator",
    "Add",
    "FAdd",
    "Sub",
    "FSub",
    "Mul",
    "FMul",
    "UDiv",
    "SDiv",
    "FDiv",
    "URem",
    "SRem",
    "FRem",
    "Shl",
    "LShr",
    "AShr",
    "And",
    "Or",
    "Xor",
    "Alloca",
    "Load",
    "Store",
    "GetElementPtr",
    "Fence",
    "AtomicCmpXchg",
    "AtomicRMW",
    "Trunc",
    "ZExt",
    "SExt",
    "FPToUI",
    "FPToSI",
    "UIToFP",
    "SIToFP",
    "FPTrunc",
    "FPExt",
    "PtrToInt",
    "IntToPtr",
    "BitCast",
    "AddrSpaceCast",
    "CleanupPad",
    "CatchPad",
    "ICmp",
    "FCmp",
    "PHI",
    "Call",
    "Select",
    "UserOp1",
    "UserOp2",
    "VAArg",
    "ExtractElement",
    "InsertElement",
    "ShuffleVector",
    "ExtractValue",
    "InsertValue",
    "FillInst",
    "ConstantInt",
    "UnknownOpc",
    "UnknownOpc",
};

enum IntegerFillType { SIGN = 0, ZEROS, ANYTHING, UNDEFINED };

enum WIAKind {
  WIAK_BINOP = 0,
  WIAK_PHI,
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
  WIAK_RET,
  WIAK_UNKNOWN
};

std::vector<std::string> WIAK_NAMES_VEC = {
    "WIAK_BINOP",         "WIAK_PHI",      "WIAK_UNOP",
    "WIAK_FILL",          "WIAK_WIDEN",    "WIAK_WIDEN_GARBAGE",
    "WIAK_NARROW",        "WIAK_DROP_EXT", "WIAK_DROP_LOCOPY",
    "WIAK_DROP_LOIGNORE", "WIAK_EXTLO",    "WIAK_SUBSUME_FILL",
    "WIAK_SUBSUME_INDEX", "WIAK_NATURAL",  "WIAK_STORE",
    "WIAK_CONSTANT",      "WIAK_LIT",      "WIAK_LOAD",
    "WIAK_UNKNOWN"};

// This class is used to specify all the arguments that a solution
// of WideningIntegerArithmetic class requires.
class WideningIntegerSolutionInfo {

public:
protected:
  // We care only about integer arithmetic instructions
  unsigned Opcode;
  // The new Opcode that has *this* as an operand
  unsigned NewOpcode;
  // Fill type for the upper bits of the expression
  // can be zero, one or garbage
  IntegerFillType FillType;     // τ
  unsigned short FillTypeWidth; // n τ[n]
  // The current width of the instruction's destination operand
  unsigned short Width; // w
  // The updated width of the instruction's destination operand
  unsigned short UpdatedWidth;
  // The cost of the solution
  short int Cost;
  // Pointer to the Value/Value that mapped to this solutionInfo
  Value *V;
  // number of operands
  short int NumOperands;

private:
  WIAKind Kind;
  // TODO: can a Value have more than 4 Operands?
  SmallPtrSet<WideningIntegerSolutionInfo *, 4> Operands;

public:
  WideningIntegerSolutionInfo() {}
  WideningIntegerSolutionInfo(WIAKind Kind_) : Kind(Kind_) {}
  WideningIntegerSolutionInfo(unsigned Opcode_, unsigned NewOpcode_,
                              IntegerFillType FillType_,
                              unsigned short FillTypeWidth_,
                              unsigned short Width_,
                              unsigned short UpdatedWidth_, short int Cost_,
                              WIAKind Kind_, Value *V_)
      : Opcode(Opcode_), NewOpcode(NewOpcode_), FillType(FillType_),
        FillTypeWidth(FillTypeWidth_), Width(Width_),
        UpdatedWidth(UpdatedWidth_), Cost(Cost_), V(V_), Kind(Kind_) {}

  WideningIntegerSolutionInfo(const WideningIntegerSolutionInfo &other)
      : Opcode(other.getOpcode()), NewOpcode(other.getNewOpcode()),
        FillType(other.getFillType()), FillTypeWidth(other.getFillTypeWidth()),
        Width(other.getWidth()), UpdatedWidth(other.getUpdatedWidth()),
        Cost(other.getCost()), V(other.getValue()), Kind(other.getKind()),
        Operands(other.getOperands()) {}

  WideningIntegerSolutionInfo(const WideningIntegerSolutionInfo &&other)
      : Opcode(other.getOpcode()), NewOpcode(other.getNewOpcode()),
        FillType(other.getFillType()), FillTypeWidth(other.getFillTypeWidth()),
        Width(other.getWidth()), UpdatedWidth(other.getUpdatedWidth()),
        Cost(other.getCost()), V(other.getValue()), Kind(other.getKind()),
        Operands(other.getOperands()) {}

  unsigned getOpcode(void) const { return Opcode; }
  void setOpcode(unsigned Opcode_) { Opcode = Opcode_; }

  unsigned getNewOpcode(void) const { return NewOpcode; }
  void setNewOpcode(unsigned NewOpcode_) { NewOpcode = NewOpcode_; }

  bool operator==(const WideningIntegerSolutionInfo &o) {
    bool MayEqual = Kind == o.getKind() && Opcode == o.getOpcode() &&
                    NewOpcode == o.getNewOpcode() && Cost == o.getCost() &&
                    Width == o.getWidth() &&
                    UpdatedWidth == o.getUpdatedWidth();
    if (MayEqual) {
      return this->getOperands() == o.getOperands();
    }
    return false;
  }

  short int getCost(void) const { return Cost; }
  void setCost(short int Cost_) { Cost = Cost_; }
  void incrementCost() { ++Cost; }
  IntegerFillType getFillType(void) const { return FillType; }
  void setFillType(IntegerFillType FillType_) { FillType = FillType_; }

  unsigned short getWidth(void) const { return Width; }
  void setWidth(unsigned short Width_) { Width = Width_; }

  unsigned short getUpdatedWidth(void) const { return UpdatedWidth; }

  void setUpdatedWidth(unsigned short UpdatedWidth_) {
    UpdatedWidth = UpdatedWidth_;
  }

  void setKind(WIAKind Kind_) { Kind = Kind_; }
  WIAKind getKind() const { return Kind; }

  void setOperands(SmallPtrSet<WideningIntegerSolutionInfo *, 4> Operands_) {
    Operands = Operands_;
    NumOperands = Operands_.size();
  }
  SmallPtrSet<WideningIntegerSolutionInfo *, 4> getOperands(void) const {
    return std::move(Operands);
  }
  void addOperand(WideningIntegerSolutionInfo *Sol) {
    Operands.insert(Sol);
    NumOperands++;
  }

  WideningIntegerSolutionInfo *getOperand(short int i) { return NULL; }

  unsigned short getFillTypeWidth(void) const { return FillTypeWidth; }

  void setFillTypeWidth(unsigned short FillType_) { FillTypeWidth = FillType_; }

  void setValue(Value *V_) { V = V_; }

  Value *getValue(void) const { return V; }

  // Returns 0 if no one is redudant
  // Returns -1 if b is redundant given solution this
  // Returns 1 if "this" is redudant given solution b
  inline int IsRedudant(const WideningIntegerSolutionInfo &b) {

    int n1 = FillTypeWidth;
    int c1 = Cost;
    int n2 = b.getFillTypeWidth();
    int c2 = b.getCost();
    int w1 = UpdatedWidth;
    int w2 = b.getUpdatedWidth();
    int FillType1 = FillType;
    int FillType2 = b.getFillType();
    if ((*this) == b) {
      return -1;
    }
    if (w1 != w2 || FillType1 != FillType2 ||
        b.getValue() != this->getValue()) {
      return 0;
    }

    // if w1 != w2 or expression1 != expression2 or fillType_A != fillType_B
    // return 0;

    if (n1 >= n2 && c1 > c2)
      return 1;
    if (n2 >= n1 && c2 >= c1)
      return -1;

    return 0;
  }

}; // WideningIntegerSolutionInfo

inline raw_ostream &operator<<(raw_ostream &out,
                               WideningIntegerSolutionInfo const &Sol) {
  out << "\tOldOpcode: " << OpcodesToStr[Sol.getOpcode()] << '\n';
  out << "\tOldOpcode in number: " << Sol.getOpcode() << '\n';
  out << "\tNewOpcode: " << OpcodesToStr[Sol.getNewOpcode()] << '\n';
  out << "\tWIA Kind : " << WIAK_NAMES_VEC[Sol.getKind()] << '\n';
  out << "\tFillType: " << Sol.getFillType() << '\n';
  out << "\tFillTypeWidth: " << Sol.getFillTypeWidth() << '\n';
  out << "\tOldWidth: " << Sol.getWidth() << '\n';
  out << "\tUpdatedWidth: " << Sol.getUpdatedWidth() << '\n';
  out << "\tCost : " << Sol.getCost() << '\n';
  int i = 0;
  for (auto Op : Sol.getOperands()) {
    out << " Children " << i << "--> " << '\n';
    out << *Op << "   ";
    i++;
  }

  return out;
}

class WIA_BINOP : public WideningIntegerSolutionInfo {
public:
  ~WIA_BINOP() {}
  WIA_BINOP() {}
  WIA_BINOP(unsigned Opcode_, unsigned NewOpcode_, IntegerFillType FillType_,
            unsigned short FillTypeWidth_, unsigned short Width_,
            unsigned short UpdatedWidth_, short int Cost_, Value *V_)
      : WideningIntegerSolutionInfo::WideningIntegerSolutionInfo(
            Opcode_, NewOpcode_, FillType_, FillTypeWidth_, Width_,
            UpdatedWidth_, Cost_, WIAK_BINOP, V_) {}

  static inline bool classof(WIA_BINOP const *) { return true; }
  static inline bool classof(WideningIntegerSolutionInfo const *Base) {
    switch (Base->getKind()) {
    case WIAK_BINOP:
      return true;
    default:
      return false;
    }
  }
};

class WIA_PHI : public WideningIntegerSolutionInfo {
public:
  ~WIA_PHI() {}
  WIA_PHI() {}
  WIA_PHI(unsigned Opcode_, unsigned NewOpcode_, IntegerFillType FillType_,
          unsigned short FillTypeWidth_, unsigned short Width_,
          unsigned short UpdatedWidth_, short int Cost_, Value *V_)
      : WideningIntegerSolutionInfo::WideningIntegerSolutionInfo(
            Opcode_, NewOpcode_, FillType_, FillTypeWidth_, Width_,
            UpdatedWidth_, Cost_, WIAK_PHI, V_) {}

  static inline bool classof(WIA_PHI const *) { return true; }
  static inline bool classof(WideningIntegerSolutionInfo const *Base) {
    switch (Base->getKind()) {
    case WIAK_PHI:
      return true;
    default:
      return false;
    }
  }
};

class WIA_FILL : public WideningIntegerSolutionInfo {
public:
  ~WIA_FILL(){};
  WIA_FILL() { ; };
  WIA_FILL(unsigned Opcode_, unsigned NewOpcode_, IntegerFillType FillType_,
           unsigned short FillTypeWidth_, unsigned short Width_,
           unsigned short UpdatedWidth_, short int Cost_, Value *V_)
      : WideningIntegerSolutionInfo::WideningIntegerSolutionInfo(
            Opcode_, NewOpcode_, FillType_, FillTypeWidth_, Width_,
            UpdatedWidth_, Cost_, WIAK_FILL, V_) {}

  static inline bool classof(WIA_FILL const *) { return true; }
  static inline bool classof(WideningIntegerSolutionInfo const *Base) {
    switch (Base->getKind()) {
    case WIAK_FILL:
      return true;
    default:
      return false;
    }
  }
};

class WIA_WIDEN : public WideningIntegerSolutionInfo {
public:
  WIA_WIDEN() {}
  ~WIA_WIDEN() {}
  WIA_WIDEN(unsigned Opcode_, unsigned NewOpcode_, IntegerFillType FillType_,
            unsigned short FillTypeWidth_, unsigned short Width_,
            unsigned short UpdatedWidth_, short int Cost_, Value *V_)
      : WideningIntegerSolutionInfo::WideningIntegerSolutionInfo(
            Opcode_, NewOpcode_, FillType_, FillTypeWidth_, Width_,
            UpdatedWidth_, Cost_, WIAK_WIDEN, V_) {}

  static inline bool classof(WIA_WIDEN const *) { return true; }
  static inline bool classof(WideningIntegerSolutionInfo const *Base) {
    switch (Base->getKind()) {
    case WIAK_WIDEN:
      return true;
    default:
      return false;
    }
  }
};

class WIA_WIDEN_GARBAGE : public WideningIntegerSolutionInfo {
public:
  WIA_WIDEN_GARBAGE() {}
  ~WIA_WIDEN_GARBAGE() {}
  WIA_WIDEN_GARBAGE(unsigned Opcode_, unsigned NewOpcode_,
                    IntegerFillType FillType_, unsigned short FillTypeWidth_,
                    unsigned short Width_, unsigned short UpdatedWidth_,
                    short int Cost_, Value *V_)
      : WideningIntegerSolutionInfo::WideningIntegerSolutionInfo(
            Opcode_, NewOpcode_, FillType_, FillTypeWidth_, Width_,
            UpdatedWidth_, Cost_, WIAK_WIDEN_GARBAGE, V_) {}

  static inline bool classof(WIA_WIDEN_GARBAGE const *) { return true; }
  static inline bool classof(WideningIntegerSolutionInfo const *Base) {
    switch (Base->getKind()) {
    case WIAK_WIDEN_GARBAGE:
      return true;
    default:
      return false;
    }
  }
};

class WIA_NARROW : public WideningIntegerSolutionInfo {
public:
  WIA_NARROW() {}
  ~WIA_NARROW() {}
  WIA_NARROW(unsigned Opcode_, unsigned NewOpcode_, IntegerFillType FillType_,
             unsigned short FillTypeWidth_, unsigned short Width_,
             unsigned short UpdatedWidth_, short int Cost_, Value *V_)
      : WideningIntegerSolutionInfo::WideningIntegerSolutionInfo(
            Opcode_, NewOpcode_, FillType_, FillTypeWidth_, Width_,
            UpdatedWidth_, Cost_, WIAK_NARROW, V_) {}

  static inline bool classof(WIA_NARROW const *) { return true; }
  static inline bool classof(WideningIntegerSolutionInfo const *Base) {
    switch (Base->getKind()) {
    case WIAK_NARROW:
      return true;
    default:
      return false;
    }
  }
};

class WIA_DROP_EXT : public WideningIntegerSolutionInfo {
public:
  WIA_DROP_EXT() {}
  ~WIA_DROP_EXT() {}
  WIA_DROP_EXT(unsigned Opcode_, unsigned NewOpcode_, IntegerFillType FillType_,
               unsigned short FillTypeWidth_, unsigned short Width_,
               unsigned short UpdatedWidth_, short int Cost_, Value *V_)
      : WideningIntegerSolutionInfo::WideningIntegerSolutionInfo(
            Opcode_, NewOpcode_, FillType_, FillTypeWidth_, Width_,
            UpdatedWidth_, Cost_, WIAK_DROP_EXT, V_) {}

  static inline bool classof(WIA_DROP_EXT const *) { return true; }
  static inline bool classof(WideningIntegerSolutionInfo const *Base) {
    switch (Base->getKind()) {
    case WIAK_DROP_EXT:
      return true;
    default:
      return false;
    }
  }
};

class WIA_DROP_LOCOPY : public WideningIntegerSolutionInfo {
public:
  WIA_DROP_LOCOPY() {}
  ~WIA_DROP_LOCOPY() {}
  WIA_DROP_LOCOPY(unsigned Opcode_, unsigned NewOpcode_,
                  IntegerFillType FillType_, unsigned short FillTypeWidth_,
                  unsigned short Width_, unsigned short UpdatedWidth_,
                  short int Cost_, Value *V_)
      : WideningIntegerSolutionInfo::WideningIntegerSolutionInfo(
            Opcode_, NewOpcode_, FillType_, FillTypeWidth_, Width_,
            UpdatedWidth_, Cost_, WIAK_DROP_LOCOPY, V_) {}

  static inline bool classof(WIA_DROP_LOCOPY const *) { return true; }
  static inline bool classof(WideningIntegerSolutionInfo const *Base) {
    switch (Base->getKind()) {
    case WIAK_DROP_LOCOPY:
      return true;
    default:
      return false;
    }
  }
};

class WIA_DROP_LOIGNORE : public WideningIntegerSolutionInfo {
public:
  WIA_DROP_LOIGNORE() {}
  ~WIA_DROP_LOIGNORE() {}
  WIA_DROP_LOIGNORE(unsigned Opcode_, unsigned NewOpcode_,
                    IntegerFillType FillType_, unsigned short FillTypeWidth_,
                    unsigned short Width_, unsigned short UpdatedWidth_,
                    short int Cost_, Value *V_)
      : WideningIntegerSolutionInfo::WideningIntegerSolutionInfo(
            Opcode_, NewOpcode_, FillType_, FillTypeWidth_, Width_,
            UpdatedWidth_, Cost_, WIAK_DROP_LOIGNORE, V_) {}

  static inline bool classof(WIA_DROP_LOIGNORE const *) { return true; }
  static inline bool classof(WideningIntegerSolutionInfo const *Base) {
    switch (Base->getKind()) {
    case WIAK_DROP_LOIGNORE:
      return true;
    default:
      return false;
    }
  }
};

class WIA_EXTLO : public WideningIntegerSolutionInfo {
public:
  WIA_EXTLO() {}
  ~WIA_EXTLO() {}
  WIA_EXTLO(unsigned Opcode_, unsigned NewOpcode_, IntegerFillType FillType_,
            unsigned short FillTypeWidth_, unsigned short Width_,
            unsigned short UpdatedWidth_, short int Cost_, Value *V_)
      : WideningIntegerSolutionInfo::WideningIntegerSolutionInfo(
            Opcode_, NewOpcode_, FillType_, FillTypeWidth_, Width_,
            UpdatedWidth_, Cost_, WIAK_EXTLO, V_) {}

  static inline bool classof(WIA_EXTLO const *) { return true; }
  static inline bool classof(WideningIntegerSolutionInfo const *Base) {
    switch (Base->getKind()) {
    case WIAK_EXTLO:
      return true;
    default:
      return false;
    }
  }
};

class WIA_SUBSUME_FILL : public WideningIntegerSolutionInfo {
public:
  WIA_SUBSUME_FILL() {}
  ~WIA_SUBSUME_FILL() {}
  WIA_SUBSUME_FILL(unsigned Opcode_, unsigned NewOpcode_,
                   IntegerFillType FillType_, unsigned short FillTypeWidth_,
                   unsigned short Width_, unsigned short UpdatedWidth_,
                   short int Cost_, Value *V_)
      : WideningIntegerSolutionInfo::WideningIntegerSolutionInfo(
            Opcode_, NewOpcode_, FillType_, FillTypeWidth_, Width_,
            UpdatedWidth_, Cost_, WIAK_SUBSUME_FILL, V_) {}

  static inline bool classof(WIA_SUBSUME_FILL const *) { return true; }
  static inline bool classof(WideningIntegerSolutionInfo const *Base) {
    switch (Base->getKind()) {
    case WIAK_SUBSUME_FILL:
      return true;
    default:
      return false;
    }
  }
};

class WIA_SUBSUME_INDEX : public WideningIntegerSolutionInfo {
public:
  WIA_SUBSUME_INDEX() {}
  ~WIA_SUBSUME_INDEX() {}
  WIA_SUBSUME_INDEX(unsigned Opcode_, unsigned NewOpcode_,
                    IntegerFillType FillType_, unsigned short FillTypeWidth_,
                    unsigned short Width_, unsigned short UpdatedWidth_,
                    short int Cost_, Value *V_)
      : WideningIntegerSolutionInfo::WideningIntegerSolutionInfo(
            Opcode_, NewOpcode_, FillType_, FillTypeWidth_, Width_,
            UpdatedWidth_, Cost_, WIAK_SUBSUME_INDEX, V_) {}

  static inline bool classof(WIA_SUBSUME_INDEX const *) { return true; }
  static inline bool classof(WideningIntegerSolutionInfo const *Base) {
    switch (Base->getKind()) {
    case WIAK_SUBSUME_INDEX:
      return true;
    default:
      return false;
    }
  }
};

class WIA_NATURAL : public WideningIntegerSolutionInfo {
public:
  WIA_NATURAL() {}
  ~WIA_NATURAL() {}
  WIA_NATURAL(unsigned Opcode_, unsigned NewOpcode_, IntegerFillType FillType_,
              unsigned short FillTypeWidth_, unsigned short Width_,
              unsigned short UpdatedWidth_, short int Cost_, Value *V_)
      : WideningIntegerSolutionInfo::WideningIntegerSolutionInfo(
            Opcode_, NewOpcode_, FillType_, FillTypeWidth_, Width_,
            UpdatedWidth_, Cost_, WIAK_NATURAL, V_) {}

  static inline bool classof(WIA_NATURAL const *) { return true; }
  static inline bool classof(WideningIntegerSolutionInfo const *Base) {

    switch (Base->getKind()) {
    case WIAK_NATURAL:
      return true;
    default:
      return false;
    }
  }
};

class WIA_LOAD : public WideningIntegerSolutionInfo {
public:
  WIA_LOAD() {}
  ~WIA_LOAD() {}
  WIA_LOAD(unsigned Opcode_, unsigned NewOpcode_, IntegerFillType FillType_,
           unsigned short FillTypeWidth_, unsigned short Width_,
           unsigned short UpdatedWidth_, short int Cost_, Value *V_)
      : WideningIntegerSolutionInfo::WideningIntegerSolutionInfo(
            Opcode_, NewOpcode_, FillType_, FillTypeWidth_, Width_,
            UpdatedWidth_, Cost_, WIAK_LOAD, V_) {}

  static inline bool classof(WIA_LOAD const *) { return true; }
  static inline bool classof(WideningIntegerSolutionInfo const *Base) {
    switch (Base->getKind()) {
    case WIAK_LOAD:
      return true;
    default:
      return false;
    }
  }
};

class WIA_STORE : public WideningIntegerSolutionInfo {
public:
  WIA_STORE() {}
  ~WIA_STORE() {}
  WIA_STORE(unsigned Opcode_, unsigned NewOpcode_, IntegerFillType FillType_,
            unsigned short FillTypeWidth_, unsigned short Width_,
            unsigned short UpdatedWidth_, short int Cost_, Value *V_)
      : WideningIntegerSolutionInfo::WideningIntegerSolutionInfo(
            Opcode_, NewOpcode_, FillType_, FillTypeWidth_, Width_,
            UpdatedWidth_, Cost_, WIAK_STORE, V_) {}

  static inline bool classof(WIA_STORE const *) { return true; }
  static inline bool classof(WideningIntegerSolutionInfo const *Base) {
    switch (Base->getKind()) {
    case WIAK_STORE:
      return true;
    default:
      return false;
    }
  }
};

class WIA_UNOP : public WideningIntegerSolutionInfo {
public:
  WIA_UNOP() {}
  ~WIA_UNOP() {}
  WIA_UNOP(unsigned Opcode_, unsigned NewOpcode_, IntegerFillType FillType_,
           unsigned short FillTypeWidth_, unsigned short Width_,
           unsigned short UpdatedWidth_, short int Cost_, Value *V_)
      : WideningIntegerSolutionInfo::WideningIntegerSolutionInfo(
            Opcode_, NewOpcode_, FillType_, FillTypeWidth_, Width_,
            UpdatedWidth_, Cost_, WIAK_UNOP, V_) {}

  static inline bool classof(WIA_UNOP const *) { return true; }
  static inline bool classof(WideningIntegerSolutionInfo const *Base) {
    switch (Base->getKind()) {
    case WIAK_UNOP:
      return true;
    default:
      return false;
    }
  }
};

class WIA_CONSTANT : public WideningIntegerSolutionInfo {
public:
  WIA_CONSTANT() {}
  ~WIA_CONSTANT() {}
  WIA_CONSTANT(unsigned Opcode_, unsigned NewOpcode_, IntegerFillType FillType_,
               unsigned short FillTypeWidth_, unsigned short Width_,
               unsigned short UpdatedWidth_, short int Cost_, Value *V_)
      : WideningIntegerSolutionInfo::WideningIntegerSolutionInfo(
            Opcode_, NewOpcode_, FillType_, FillTypeWidth_, Width_,
            UpdatedWidth_, Cost_, WIAK_CONSTANT, V_) {}

  static inline bool classof(WIA_CONSTANT const *) { return true; }
  static inline bool classof(WideningIntegerSolutionInfo const *Base) {
    switch (Base->getKind()) {
    case WIAK_CONSTANT:
      return true;
    default:
      return false;
    }
  }
};

} // namespace llvm

#endif // LLVM_CODEGEN_WIDENINGINTEGERARITHMETICINFO_H
