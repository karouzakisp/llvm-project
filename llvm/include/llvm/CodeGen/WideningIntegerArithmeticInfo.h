#ifndef LLVM_CODEGEN_WIDENINGINTEGERARITHMETICINFO_H
#define LLVM_CODEGEN_WIDENINGINTEGERARITHMETICINFO_H


#include "llvm/IR/Instruction.h"
#include "llvm/ADT/SmallPtrSet.h"


namespace llvm {


class Instruction;

enum IntegerFillType{
  SIGN = 0,
  ZEROS,
  ANYTHING,
  UNDEFINED
};



// This class is used to specify all the arguments that a solution
// of WideningIntegerArithmetic class requires.
class WideningIntegerSolutionInfo{
public:

  enum WIAKind{
    WIAK_BINOP= 0,
    WIAK_FILL,
    WIAK_WIDEN_ZERO,
    WIAK_WIDEN_SIGN,
    WIAK_WIDEN_GARBAGE,
    WIAK_NARROW,
    WIAK_DROP_EXT,
    WIAK_DROP_LOCOPY,
    WIAK_DROP_LOIGNORE,
    WIAK_EXTLO,
    WIAK_SUBSUME_FILL,
    WIAK_SUBSUME_INDEX,
    WIAK_NATURAL
  };

  WideningIntegerSolutionInfo(WIADerivedId id): Id(id) {} 
  WideningIntegerSolutionInfo(unsigned char Opcode_, IntegerFillType FillType_,
  unsigned FillTypeWidth_ unsigned char Width_, unsigned char UpdatedWidth_, 
            short int Cost_, WIADerivedId id): 
  Width(Width_), FillType(FillType_), FillTypeWidth(FillTypeWidth_), 
  UpdatedWidth(UpdatedWidth_),
  Cost(Cost_), Opcode(Opcode_), Id(id) {}
 
  WideningIntegerSolutionInfo(const WideningIntegerSolutionInfo &other)
  : Width(other.getWidth()), FillType(other.getFillType()),
    FillTypeWidth(other.getFillTypeWidth()), 
    UpdatedWidth(other.getUpdatedWidth()), Cost(other.getCost()),
    Opcode(other.getOpcode()), Kind(other.getKind()) 
  {
    this.setOperands(other.getOperands());
  }  
 
  virtual inline bool operator==(const WideningIntegerSolutionInfo &a) = 0; 


  unsigned getOpcode(void) const {
    return Opcode;
  }
  void setOpcode(unsigned Opcode_){
    Opcode = Opcode_;
  }  

  unsigned getUpdatedOpcode(void) const {
    return UpdatedOpcode;
  }
  void setUpdatedOpcode(unsigned UpdatedOpcode_){
    UpdatedOpcode = UpdatedOpcode_;
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

  unsigned char getWidth(void) const {
    return Width;
  }
  void setWidth(unsigned char Width_){
    Width = Width_;
  }
  
  unsigned char getUpdatedWidth(void) const {
    return UpdatedWidth;
  }

  void setUpdatedWidth(unsigned char UpdatedWidth_){
    UpdatedWidth = UpdatedWidth_;
  }
  
  void getKind() const {
    return Kind;
  }
  
  void setOperands(SmallPtrSetImpl<WideningIntegerSolutionInfo *>  Operands_){
    this.Operands = Operands_;
  }
  SmallPtrSetImpl<WideningIntegerSolutionInfo *>  getOperands(void) const {
    return Operands;
  }
  void addOperand(WideningIntegerSolutionInfo *Sol){
    Operands.insert(Sol);
  }
  
  unsigned  getFillTypeWidth(void) const {
    return FillTypeWidth;
  }

  void setFillTypeWidth(unsigned FillType_){
    this.FillTypeWidth = FillType_;
  }
 
  // Returns 0 if no one is redudant
  // Returns -1 if b is redundant given solution this
  // Returns 1 if "this" is redudant given solution b
  inline int isRedudant(const WideningIntegerSolutionInfo &b){

    int n1 = this.width;
    int c1 = this.Cost;
    int n2 = b.getWidth();
    int c2 = b.getCost();
    
  
    if(n1 >= n2  && c1 >= c2 )
      return 1;
    if(n2 >= n1 && c2 >= c1 )
      return -1;
   
    return 0; 
  }

protected:
  

  // We care only about integer arithmetic instructions
  unsigned       Opcode;
  
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

  
  // The updated instruction if we add an extension
  unsigned       UpdatedOpcode;
  
private:  
  WIAKind Kind;
  // TODO can a SDNode have more than 4 Operands? 
  SmallPtrSet<WideningIntegerSolutionInfo *, 4> Operands;
}; // WideningIntegerSolutionInfo


class WIA_BINOP : public WideningIntegerSolutionInfo
{
  public:
  ~WIA_BINOP() {}
  WIA_BINOP() {}
  WIA_BINOP(WideningIntegerSolutionInfo *S_): S(S_) {}
  WIA_BINOP(unsigned char Opcode_, IntegerFillType FillType_,
            unsigned char FillTypeWidth_,
            unsigned char Width_, unsigned char UpdatedWidth_, 
            short int Cost_): 
      WideningIntegerSolutionInfo::WideningIntegerSolutionInfo(
        Opcode_, FillType_, FillTypeWidth_, Width_, UpdatedWidth_, Cost_, 
        WIAK_BINOP) {}
  
  static inline bool classof(WIA_BINOP const *) { return true}
  static inline bool classof(WideningIntegerSolutionInfo const *Base){
    switch(Base->getKind()){
      case WIAK_BINOP: return true;
      default: return false;
    } 
  }
  
  
  virtual inline bool operator==(const WideningIntegerSolutionInfo& a) override
  {
    return (isa<WIA_BINOP>(&a));
      
  }
  
  
};


class WIA_FILL : public WideningIntegerSolutionInfo
{
  public:
  ~WIA_FILL() {};
  WIA_FILL() {;};
  WIA_FILL(WideningIntegerSolutionInfo *S_): S(S_) {}
  WIA_FILL(unsigned char Opcode_, IntegerFillType FillType_,
            unsigned char FillTypeWidth_,
            unsigned char Width_, unsigned char UpdatedWidth_, 
            short int Cost_): 
      WideningIntegerSolutionInfo::WideningIntegerSolutionInfo(
        Opcode_, FillType_, FillTypeWidth_Width_, UpdatedWidth_, Cost_,
        WIAK_FILL) {}: 

  static inline bool classof(WIA_FILL const *) { return true}
  static inline bool classof(WideningIntegerSolutionInfo const *Base){
    switch(Base->getKind()){
      case WIAK_FILL: return true;
      default: return false;
    } 
  }
  virtual bool operator==(const WideningIntegerSolutionInfo& a) override {
    return (isa<WIA_FILL>(&a));
  }
};


class WIA_WIDEN : public WideningIntegerSolutionInfo
{
  public:
  WIA_WIDEN() {}
  ~WIA_WIDEN() {}
  WIA_WIDEN(WideningIntegerSolutionInfo *S_): S(S_) {}
  WIA_WIDEN(unsigned char Opcode_, IntegerFillType FillType_,
            unsigned char FillTypeWidth_,
            unsigned char Width_, unsigned char UpdatedWidth_, 
            short int Cost_): 
      WideningIntegerSolutionInfo::WideningIntegerSolutionInfo(
        Opcode_, FillType_, FillTypeWidth_, Width_, UpdatedWidth_, Cost_, 
        WIAK_WIDEN) {}
  
  static inline bool classof(WIA_WIDEN const *) { return true}
  static inline bool classof(WideningIntegerSolutionInfo const *Base){
    switch(Base->getKind()){
      case WIAK_WIDEN: return true;
      default: return false;
    } 
  }

  
  virtual bool operator==(const WideningIntegerSolutionInfo& a) override {
    return (isa<WIA_WIDEN>(&a));
  }
};

class WIA_WIDEN_GARBAGE : public WideningIntegerSolutionInfo
{
  public:
  WIA_WIDEN_GARBAGE() {}
  ~WIA_WIDEN_GARBAGE() {}
  WIA_WIDEN_GARBAGE(WideningIntegerSolutionInfo *S_): S(S_) {}
  WIA_WIDEN_GARBAGE(unsigned char Opcode_, IntegerFillType FillType_,
            unsigned char FillTypeWidth_,
            unsigned char Width_, unsigned char UpdatedWidth_, 
            short int Cost_): 
      WideningIntegerSolutionInfo::WideningIntegerSolutionInfo(
        Opcode_, FillType_, FillTypeWidth_, Width_, UpdatedWidth_, Cost_,
        WIAK_WIDEN_GARBAGE) {}

  static inline bool classof(WIA_WIDEN_GARBAGE const *) { return true}
  static inline bool classof(WideningIntegerSolutionInfo const *Base){
    switch(Base->getKind()){
      case WIAK_WIDEN_GARBAGE: return true;
      default: return false;
    } 
  }
  
  virtual bool operator==(const WideningIntegerSolutionInfo& a) override {
    return (isa<WIA_WIDEN_GARBAGE>(&a));
  }
};

class WIA_NARROW : public WideningIntegerSolutionInfo
{
  public:
  WIA_NARROW() {}
  ~WIA_NARROW() {}
  WIA_NARROW(WideningIntegerSolutionInfo *S_): S(S_) {}
  WIA_NARROW(unsigned char Opcode_, IntegerFillType FillType_,
            unsigned char FillTypeWidth_,
            unsigned char Width_, unsigned char UpdatedWidth_, 
            short int Cost_): 
      WideningIntegerSolutionInfo::WideningIntegerSolutionInfo(
        Opcode_, FillType_ , FillTypeWidth , Width_, UpdatedWidth_, Cost_,
        WIAK_NARROW) {}

  static inline bool classof(WIA_NARROW const *) { return true}
  static inline bool classof(WideningIntegerSolutionInfo const *Base){
    switch(Base->getKind()){
      case WIAK_NARROW: return true;
      default: return false;
    } 
  }
  
  virtual bool operator==(const WideningIntegerSolutionInfo& a) override {
    return (isa<WIA_NARROW>(&a));
  }
};


class WIA_DROP_EXT : public WideningIntegerSolutionInfo
{
  public:
  WIA_DROP_EXT() {}
  ~WIA_DROP_EXT() {}
  WIA_DROP_EXT(WideningIntegerSolutionInfo *S_): S(S_) {}
  WIA_DROP_EXT(unsigned char Opcode_, IntegerFillType FillType_,
            unsigned char FillTypeWidth_,
            unsigned char Width_, unsigned char UpdatedWidth_, 
            short int Cost_): 
      WideningIntegerSolutionInfo::WideningIntegerSolutionInfo(
        Opcode_, FillType_, FillTypeWidth_, Width_, UpdatedWidth_, Cost_,
        WIAK_DROP_EXT) {}

  static inline bool classof(WIA_DROP_EXT const *) { return true}
  static inline bool classof(WideningIntegerSolutionInfo const *Base){
    switch(Base->getKind()){
      case WIAK_DROP_EXT: return true;
      default: return false;
    } 
  }
  
  virtual bool operator==(const WideningIntegerSolutionInfo& a) override {
    return (isa<WIA_DROP_EXT>(&a));
  }
};


class WIA_DROP_LOCOPY : public WideningIntegerSolutionInfo
{
  public:
  WIA_DROP_LOCOPY() {}
  ~WIA_DROP_LOCOPY() {}
  WIA_DROP_LOCOPY(WideningIntegerSolutionInfo *S_): S(S_) {}
  WIA_DROP_LOCOPY(unsigned char Opcode_, IntegerFillType FillType_,
            unsigned char FillTypeWidth_, 
            unsigned char Width_, unsigned char UpdatedWidth_, 
            short int Cost_): 
      WideningIntegerSolutionInfo::WideningIntegerSolutionInfo(
        Opcode_, FillType_, FillTypeWidth_, Width_, UpdatedWidth_, Cost_,
        WIAK_DROP_LOCOPY) {}

  static inline bool classof(WIA_DROP_LOCOPY const *) { return true}
  static inline bool classof(WideningIntegerSolutionInfo const *Base){
    switch(Base->getKind()){
      case WIAK_DROP_LOCOPY: return true;
      default: return false;
    } 
  }
  
  virtual bool operator==(const WideningIntegerSolutionInfo& a) override {
    return (isa<WIA_DROP_LOCOPY>(&a));
  }
};


class WIA_DROP_LOIGNORE : public WideningIntegerSolutionInfo
{
  public:
  WIA_DROP_LOIGNORE() {}
  ~WIA_DROP_LOIGNORE() {}
  WIA_DROP_LOIGNORE(WideningIntegerSolutionInfo *S_): S(S_) {}
  WIA_DROP_LOIGNORE(unsigned char Opcode_, IntegerFillType FillType_,
            unsigned char FillTypeWidth_,
            unsigned char Width_, unsigned char UpdatedWidth_, 
            short int Cost_): 
      WideningIntegerSolutionInfo::WideningIntegerSolutionInfo(
        Opcode_, FillType_, FillTypeWidth_, Width_, UpdatedWidth_, Cost_,
        WIAK_DROP_LOIGNORE) {}

  static inline bool classof(WIA_DROP_LOIGNORE const *) { return true}
  static inline bool classof(WideningIntegerSolutionInfo const *Base){
    switch(Base->getKind()){
      case WIAK_DROP_LOIGNORE: return true;
      default: return false;
    } 
  }
  
  virtual bool operator==(const WideningIntegerSolutionInfo& a) override {
    return (isa<WIA_DROP_LOIGNORE>(&a));
  }
};


class WIA_EXTLO : public WideningIntegerSolutionInfo
{
  public:
  WIA_EXTLO() {}
  ~WIA_EXTLO() {}
  WIA_EXTLO(WideningIntegerSolutionInfo *S_): S(S_) {}
  WIA_EXTLO(unsigned char Opcode_, IntegerFillType FillType_,
            unsigned char FillTypeWidth_,
            unsigned char Width_, unsigned char UpdatedWidth_, 
            short int Cost_): 
      WideningIntegerSolutionInfo::WideningIntegerSolutionInfo(
        Opcode_, FillType_, FillTypeWidth_, Width_, UpdatedWidth_, Cost_,
        WIAK_EXTLO) {}

  static inline bool classof(WIA_EXTLO const *) { return true}
  static inline bool classof(WideningIntegerSolutionInfo const *Base){
    switch(Base->getKind()){
      case WIAK_EXTLO: return true;
      default: return false;
    } 
  }
  
  virtual bool operator==(const WideningIntegerSolutionInfo& a) override {
    return (isa<WIA_EXTLO>(&a));
  }
};


class WIA_SUBSUME_FILL : public WideningIntegerSolutionInfo
{
  public:
  WIA_SUBSUME_FILL() {}
  ~WIA_SUBSUME_FILL() {}
  WIA_SUBSUME_FILL(WideningIntegerSolutionInfo *S_): S(S_) {}
  WIA_SUBSUME_FILL(unsigned char Opcode_, IntegerFillType FillType_,
            unsigned char FillTypeWidth_,
            unsigned char Width_, unsigned char UpdatedWidth_, 
            short int Cost_): 
      WideningIntegerSolutionInfo::WideningIntegerSolutionInfo(
        Opcode_, FillType_, FillTypeWidth_, Width_, UpdatedWidth_, Cost_,
        WIAK_SUBSUME_FILL) {}

  static inline bool classof(WIA_SUBSUME_FILL const *) { return true}
  static inline bool classof(WideningIntegerSolutionInfo const *Base){
    switch(Base->getKind()){
      case WIAK_SUBSUME_FILL: return true;
      default: return false;
    } 
  }
  
  virtual bool operator==(const WideningIntegerSolutionInfo& a) override {
    return (isa<WIA_SUBSUME_FILL>(&a));
  }
};


class WIA_SUBSUME_INDEX : public WideningIntegerSolutionInfo
{
  public:
  WIA_SUBSUME_INDEX() {}
  ~WIA_SUBSUME_INDEX() {}
  WIA_SUBSUME_INDEX(WideningIntegerSolutionInfo *S_): S(S_) {}
  WIA_SUBSUME_INDEX(unsigned char Opcode_, IntegerFillType FillType_,
            unsigned char FillTypeWidth_,
            unsigned char Width_, unsigned char UpdatedWidth_, 
            short int Cost_, WIAKind id = WIAK_SUBSUME_INDEX): 
      WideningIntegerSolutionInfo::WideningIntegerSolutionInfo(
        Opcode_, FillType_, FillTypeWidth_, Width_, UpdatedWidth_, Cost_, id) {}

  static inline bool classof(WIA_SUBSUME_INDEX const *) { return true}
  static inline bool classof(WideningIntegerSolutionInfo const *Base){
    switch(Base->getKind()){
      case WIAK_SUBSUME_INDEX: return true;
      default: return false;
    } 
  }
  
  virtual bool operator==(const WideningIntegerSolutionInfo& a) override {
    return (isa<WIA_SUBSUME_INDEX>(&a));
  }
};


class WIA_NATURAL : public WideningIntegerSolutionInfo
{
  public:
  WIA_NATURAL() {}
  ~WIA_NATURAL() {}
  WIA_NATURAL(WideningIntegerSolutionInfo *S_): S(S_) {}
  WIA_NATURAL(unsigned char Opcode_, IntegerFillType FillType_,
            unsigned char FillTypeWidth_,
            unsigned char Width_, unsigned char UpdatedWidth_, 
            short int Cost_, WIAKind = WIAK_NATURAL): 
      WideningIntegerSolutionInfo::WideningIntegerSolutionInfo(
        Opcode_, FillType_, FillTypeWidth_, Width_, UpdatedWidth_, Cost_, id) {}

  static inline bool classof(WIAK_NATURAL const *) { return true}
  static inline bool classof(WideningIntegerSolutionInfo const *Base){
    switch(Base->getKind()){
      case WIAK_NATURAL: return true;
      default: return false;
    } 
  }
  
  virtual bool operator==(const WideningIntegerSolutionInfo& a) override {
    return (isa<WIA_SUBSUME_INDEX>(&a));
  }
};






} // namespace llvm;


#endif // LLVM_ANALYSIS_UTILS_WIDENINGINTEGERARITHMETIC_H
