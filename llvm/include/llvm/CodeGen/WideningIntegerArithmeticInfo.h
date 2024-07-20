#ifndef LLVM_CODEGEN_WIDENINGINTEGERARITHMETICINFO_H
#define LLVM_CODEGEN_WIDENINGINTEGERARITHMETICINFO_H


#include "llvm/IR/Instruction.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/CodeGen/SelectionDAGNodes.h"

namespace llvm {


class Instruction;

enum IntegerFillType{
  SIGN = 0,
  ZEROS,
  ANYTHING,
  UNDEFINED
};
  
enum WIAKind{
  WIAK_BINOP= 0,
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
  WIAK_ASSIGN,
  WIAK_VAR,
  WIAK_LIT,
};



// This class is used to specify all the arguments that a solution
// of WideningIntegerArithmetic class requires.
class WideningIntegerSolutionInfo{

public:


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
   
 
  virtual inline bool operator==(const WideningIntegerSolutionInfo &a); 


  unsigned getOpcode(void) const {
    return Opcode;
  }
  void setOpcode(unsigned Opcode_){
    Opcode = Opcode_;
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
  inline int IsRedudant(const WideningIntegerSolutionInfo *b){

    int n1 = Width;
    int c1 = Cost;
    int n2 = b->getWidth();
    int c2 = b->getCost();
    
  
    if(n1 >= n2  && c1 >= c2 )
      return 1;
    if(n2 >= n1 && c2 >= c1 )
      return -1;
   
    return 0; 
  }

}; // WideningIntegerSolutionInfo


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
  virtual bool operator==(const WideningIntegerSolutionInfo& a) override {
    return (isa<WIA_FILL>(&a));
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

  
  virtual bool operator==(const WideningIntegerSolutionInfo& a) override {
    return (isa<WIA_WIDEN>(&a));
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
  
  virtual bool operator==(const WideningIntegerSolutionInfo& a) override {
    return (isa<WIA_WIDEN_GARBAGE>(&a));
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
        Opcode_, FillType_ , FillTypeWidth , Width_, 
        UpdatedWidth_, Cost_, WIAK_NARROW, Node_){}

  static inline bool classof(WIA_NARROW const *) { return true; }
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
  
  virtual bool operator==(const WideningIntegerSolutionInfo& a) override {
    return (isa<WIA_DROP_EXT>(&a));
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
  
  virtual bool operator==(const WideningIntegerSolutionInfo& a) override {
    return (isa<WIA_DROP_LOCOPY>(&a));
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
  
  virtual bool operator==(const WideningIntegerSolutionInfo& a) override {
    return (isa<WIA_DROP_LOIGNORE>(&a));
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
  
  virtual bool operator==(const WideningIntegerSolutionInfo& a) override {
    return (isa<WIA_EXTLO>(&a));
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
  
  virtual bool operator==(const WideningIntegerSolutionInfo& a) override {
    return (isa<WIA_SUBSUME_FILL>(&a));
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
  
  virtual bool operator==(const WideningIntegerSolutionInfo& a) override {
    return (isa<WIA_SUBSUME_INDEX>(&a));
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
  
  virtual bool operator==(const WideningIntegerSolutionInfo& a) override {
    return (isa<WIA_NATURAL>(&a));
  }
};


class WideningIntegerSolInfoBuilder {
  public:
  WideningIntegerSolutionInfo* build(WIAKind Kind){
    switch(Kind){
      default:
        return new WideningIntegerSolutionInfo();
      case WIAK_BINOP:
        return new WIA_BINOP();
      case WIAK_FILL:
        return new WIA_FILL();
      case WIAK_WIDEN:
        return new WIA_WIDEN();
      case WIAK_WIDEN_GARBAGE:
        return new WIA_WIDEN_GARBAGE();
      case WIAK_NARROW:
        return new WIA_NARROW();
      case WIAK_DROP_EXT:
        return new WIA_DROP_EXT();
      case WIAK_DROP_LOCOPY:
        return new WIA_DROP_LOCOPY();
      case WIAK_DROP_LOIGNORE:
        return new WIA_DROP_LOIGNORE();
      case WIAK_EXTLO:
        return new WIA_EXTLO();
      case WIAK_SUBSUME_FILL:
        return new WIA_SUBSUME_FILL();
      case WIAK_SUBSUME_INDEX:
        return new WIA_SUBSUME_INDEX();
      case WIAK_NATURAL:
        return new WIA_NATURAL();

    }
  }

};





} // namespace llvm;


#endif // LLVM_ANALYSIS_UTILS_WIDENINGINTEGERARITHMETIC_H
