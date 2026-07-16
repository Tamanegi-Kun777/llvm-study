#ifndef AST_HPP
#define AST_HPP


#include <string>
#include <map>
#include <vector>
#include "llvm/Support/Casting.h"
#include "APP.hpp"

enum AstID
{
  BaseID,
  VariableDeclID,
  BinaryExprID,
  NullExprID,
  CallExprID,
  JumpStmtID,
  VariableID,
  NumberID,
  IfStmtID,
  WhileStmtID,
  ForStmtID,
  StructDeclID,
  MemberAccessID
};
// ファイルの、先頭あたりに、追加
class PrototypeAST;
class FunctionAST;
class FunctionStmtAST;
/*
 *   ASTの基底クラス
 */
class BaseAST
{
  AstID ID;

public:
  BaseAST(AstID id) : ID(id){};
  virtual ~BaseAST(){};
  AstID getValueID() const {return ID;};
};

// ; を示すAST
class NullExprAST : public BaseAST
{
public:
  NullExprAST() : BaseAST(NullExprID){}

  static inline bool classof(NullExprAST const*){return true;}

  static inline bool classof(BaseAST const* base){
    return base->getValueID() == NullExprID;
  }
};

/*
 *   変数宣言を表すAST
 */
class VariableDeclAST : public BaseAST
{
public:
  typedef enum{
    param,
    local
  }DeclType;

private:
  std::string Name;
  std::string TypeName;
  DeclType Type;

public:
  VariableDeclAST(const std::string &name, const std::string &type_name = "int") : BaseAST(VariableDeclID), Name(name), TypeName(type_name){};

  // VariableDeclASTなのでtrueを返す
  static inline bool classof(VariableDeclAST const*){return true;};

  // 渡されたBaseASTクラスがVariableDeclASTか判定する
  static inline bool classof(BaseAST const* base){
    return base->getValueID() == VariableDeclID;
  };

  ~VariableDeclAST(){};

  // 変数名を取得する
  std::string getName(){return Name;};
  std::string getTypeName(){return TypeName;};

  // 変数の宣言種別を設定する
  bool setDeclType(DeclType type){
    Type = type;
    return true;
  };

  // 変数の宣言種別を取得する
  DeclType getType(){return Type;};
};

/*
 *   二項演算を表すAST
 */
class BinaryExprAST : public BaseAST
{
  std::string Op;
  BaseAST *LHS, *RHS;

public:
  BinaryExprAST(std::string op, BaseAST *lhs, BaseAST *rhs) : BaseAST(BinaryExprID), Op(op), LHS(lhs), RHS(rhs){};

  ~BinaryExprAST(){SAFE_DELETE(LHS); SAFE_DELETE(RHS);};

  // BinaryExprASTなのでtrueを返す
  static inline bool classof(BinaryExprAST const*){return true;};

  // 渡されたBaseASTがBinaryExprASTか判定する
  static inline bool classof(BaseAST const* base){
    return base->getValueID() == BinaryExprID;
  };

  // 演算子を取得する
  std::string getOp(){return Op;};

  // 左辺値を取得する
  BaseAST *getLHS(){return LHS;};

  // 右辺値を取得する
  BaseAST *getRHS(){return RHS;};
};

/*
 *  関数呼び出しを表すAST
 */
class CallExprAST : public BaseAST
{
  std::string Callee;
  std::vector<BaseAST*> Args;

public:
  CallExprAST(const std::string &callee, std::vector<BaseAST*> &args)
    : BaseAST(CallExprID), Callee(callee), Args(args){};

  ~CallExprAST();

  // CallExprASTなのでtrueを返す
  static inline bool classof(CallExprAST const*){return true;};

  // 渡されたBaseASTがCallExprASTなのか判定する
  static inline bool classof(BaseAST const* base){
    return base->getValueID() == CallExprID;
  };

  // 呼び出す関数名を取得する
  std::string getCallee(){return Callee;};

  // i番目の引数を取得する
  BaseAST *getArgs(int i){
    if(i<Args.size()){
      return Args.at(i);
    }
    else{
      return NULL;
    }
  };
};
/*
 *  ジャンプ（ここではreturn）を表すAST
 */
class JumpStmtAST : public BaseAST
{
  BaseAST *Expr;

public:
  JumpStmtAST(BaseAST *expr) : BaseAST(JumpStmtID), Expr(expr){};

  ~JumpStmtAST(){SAFE_DELETE(Expr);};

  static inline bool classof(JumpStmtAST const*){return true;};

  static inline bool classof(BaseAST const* base){
    return base->getValueID() == JumpStmtID;
  };

  BaseAST *getExpr(){return Expr;};
};
/*
 *  if文を表すAST
 */
class IfStmtAST : public BaseAST
{
  BaseAST *Condition;
  std::vector<BaseAST*> ThenStmts;
  std::vector<BaseAST*> ElseStmts;
public:
  IfStmtAST(BaseAST *condition) : BaseAST(IfStmtID), Condition(condition){};
  ~IfStmtAST(){SAFE_DELETE(Condition);};
  static inline bool classof(IfStmtAST const*){return true;};
  static inline bool classof(BaseAST const* base){
    return base->getValueID() == IfStmtID;
  };
  BaseAST *getCondition(){return Condition;};
  bool addThenStmt(BaseAST *stmt){ThenStmts.push_back(stmt); return true;};
  bool addElseStmt(BaseAST *stmt){ElseStmts.push_back(stmt); return true;};
  BaseAST *getThenStmt(int i){
    if(i < ThenStmts.size()){ return ThenStmts.at(i); }
    else{ return NULL; }
  };
  BaseAST *getElseStmt(int i){
    if(i < ElseStmts.size()){ return ElseStmts.at(i); }
    else{ return NULL; }
  };
};
/*
 *  while文を表すAST
 */
class WhileStmtAST : public BaseAST
{
  BaseAST *Condition;
  std::vector<BaseAST*> BodyStmts;
public:
  WhileStmtAST(BaseAST *condition) : BaseAST(WhileStmtID), Condition(condition){};
  ~WhileStmtAST(){SAFE_DELETE(Condition);};
  static inline bool classof(WhileStmtAST const*){return true;};
  static inline bool classof(BaseAST const* base){
    return base->getValueID() == WhileStmtID;
  };
  BaseAST *getCondition(){return Condition;};
  bool addBodyStmt(BaseAST *stmt){BodyStmts.push_back(stmt); return true;};
  BaseAST *getBodyStmt(int i){
    if(i < BodyStmts.size()){ return BodyStmts.at(i); }
    else{ return NULL; }
  };
};
/*
 *  for文を表すAST
 */
class ForStmtAST : public BaseAST
{
  BaseAST *Init;
  BaseAST *Condition;
  BaseAST *Update;
  std::vector<BaseAST*> BodyStmts;
public:
  ForStmtAST(BaseAST *init, BaseAST *condition, BaseAST *update)
    : BaseAST(ForStmtID), Init(init), Condition(condition), Update(update){};
  ~ForStmtAST(){
    SAFE_DELETE(Init);
    SAFE_DELETE(Condition);
    SAFE_DELETE(Update);
  };
  static inline bool classof(ForStmtAST const*){return true;};
  static inline bool classof(BaseAST const* base){
    return base->getValueID() == ForStmtID;
  };
  BaseAST *getInit(){return Init;};
  BaseAST *getCondition(){return Condition;};
  BaseAST *getUpdate(){return Update;};
  bool addBodyStmt(BaseAST *stmt){BodyStmts.push_back(stmt); return true;};
  BaseAST *getBodyStmt(int i){
    if(i < BodyStmts.size()){ return BodyStmts.at(i); }
    else{ return NULL; }
  };
};
/*
 *  構造体定義を表すAST
 */
class StructDeclAST : public BaseAST
{
  std::string Name;
  std::vector<std::string> MemberNames;
  std::vector<std::string> MemberTypes;
public:
  StructDeclAST(const std::string &name) : BaseAST(StructDeclID), Name(name){};
  ~StructDeclAST(){};
  static inline bool classof(StructDeclAST const*){return true;};
  static inline bool classof(BaseAST const* base){
    return base->getValueID() == StructDeclID;
  };
  std::string getName(){return Name;};
  bool addMember(const std::string &member_name, const std::string &member_type){
    MemberNames.push_back(member_name);
    MemberTypes.push_back(member_type);
    return true;
  };
  int getMemberNum(){return MemberNames.size();};
  std::string getMemberName(int i){
    if(i < MemberNames.size()){ return MemberNames.at(i); }
    else{ return ""; }
  };
  std::string getMemberType(int i){
    if(i < MemberTypes.size()){ return MemberTypes.at(i); }
    else{ return ""; }
  };
};
/*
 *  メンバアクセス(p.x)を表すAST
 */
class MemberAccessAST : public BaseAST
{
  std::string VariableName;
  std::string MemberName;
public:
  MemberAccessAST(const std::string &var_name, const std::string &member_name)
    : BaseAST(MemberAccessID), VariableName(var_name), MemberName(member_name){};
  ~MemberAccessAST(){};
  static inline bool classof(MemberAccessAST const*){return true;};
  static inline bool classof(BaseAST const* base){
    return base->getValueID() == MemberAccessID;
  };
  std::string getVariableName(){return VariableName;};
  std::string getMemberName(){return MemberName;};
};
/*
 *  変数参照を表すAST
 */

class VariableAST : public BaseAST
{
  std::string Name;

public:
  VariableAST(const std::string &name) : BaseAST(VariableID), Name(name){};

  ~VariableAST(){};

  // VariableASTなのでtrueを返す
  static inline bool classof(VariableAST const*){return true;};

  // 渡されたBAせASTがVariableASTなのか判定する
  static inline bool classof(BaseAST const* base){
    return base->getValueID() == VariableID;
  };

  // 変数名を取得する
  std::string getName(){return Name;};
};

/*
 *  整数を表すAST
 */

class NumberAST : public BaseAST
{
  int Val;

public:
  NumberAST(int val) : BaseAST(NumberID), Val(val){};

  ~NumberAST(){};

  //NumberASTなのでtrueを返す
  static inline bool classof(NumberAST const*){return true;};

  //渡されたBaseASTがNumberASTか判定する
  static inline bool classof(BaseAST const* base){
    return base->getValueID() == NumberID;
  };

  //このASTは表現する値を取得する
  int getNumberValue(){return Val;};
};
/*
 *  ソースコードを表すAST
 */
class TranslationUnitAST
{
  std::vector<PrototypeAST*> Prototypes;
  std::vector<FunctionAST*> Functions;
  std::vector<StructDeclAST*> Structs;

public:
  TranslationUnitAST(){};
  ~TranslationUnitAST();

  //モジュールにプロトタイプ宣言を追加する
  bool addPrototype(PrototypeAST *proto);

  //モジュールに関数を追加する
  bool addFunction(FunctionAST *func);
  //モジュールに構造体定義を追加する
  bool addStruct(StructDeclAST *s){Structs.push_back(s); return true;}
  //モジュールが空か判定する
  bool empty();

  //i番目のプロトタイプ宣言を取得する
  PrototypeAST *getPrototype(int i){
    if(i < Prototypes.size()){
      return Prototypes.at(i);
    }
    else{
      return NULL;
    }
  }

  //i番目の関数を取得する
  FunctionAST *getFunction(int i){
    if(i < Functions.size()){
      return Functions.at(i);
    }
    else{
      return NULL;
    }
  }
  //i番目の構造体定義を取得する
  StructDeclAST *getStruct(int i){
    if(i < Structs.size()){
      return Structs.at(i);
    }
    else{
      return NULL;
    }
  }
};

/*
 *  関数宣言を表すAST
 */
class PrototypeAST
{
  std::string Name;
  std::vector<std::string> Params;

public:
  PrototypeAST(const std::string &name, const std::vector<std::string> &params)
    : Name(name), Params(params){};

  //関数名を取得する
  std::string getName(){return Name;};

  //i番目の引数名を取得する
  std::string getParamName(int i){
    if(i < Params.size()){
      return Params.at(i);
    }
    else{
      return NULL;
    }
  }
  ;
  //引数の数を取得する
  int getParamNum(){return Params.size();};
};

/*
 *  関数定義を表すAST
 */
class FunctionAST
{
  PrototypeAST *Proto;
  FunctionStmtAST *Body;

public:
  FunctionAST(PrototypeAST *proto, FunctionStmtAST * body) : Proto(proto), Body(body){};

  ~FunctionAST();
  //関数名を取得する
  std::string getName(){return Proto->getName();};
  
  //この関数のプロトタイプ宣言を取得する
  PrototypeAST *getPrototype(){return Proto;};
  
  //この関数のボディを取得する
  FunctionStmtAST *getBody(){return Body;};
};

/*
 *  関数定義（ボディ）を表すAST
 */

class FunctionStmtAST
{
  std::vector<VariableDeclAST*> VariableDecls;
  std::vector<BaseAST*> StmtLists;

public:
  FunctionStmtAST(){};

  ~FunctionStmtAST();

  //関数に変数を追加する
  bool addVariableDeclaration(VariableDeclAST *vdecl);

  //関数にステートメントを追加する
  bool addStatement(BaseAST *stmt){StmtLists.push_back(stmt);return true;}

  //i番目の変数を取得する
  VariableDeclAST *getVariableDecl(int i){
    if(i < VariableDecls.size()){
      return VariableDecls.at(i);
    }
    else{
      return NULL;
    }
  };

  //i番目のステートメントを取得する
  BaseAST *getStatement(int i){
    if(i < StmtLists.size()){
      return StmtLists.at(i);
    }
    else{
      return NULL;
    }
  };
};

#endif