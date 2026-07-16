#include "codegen.hpp"

CodeGen::CodeGen(){
  Builder = new llvm::IRBuilder<>(Context);
  Mod = NULL;
}

CodeGen::~CodeGen(){
	SAFE_DELETE(Builder);
	SAFE_DELETE(Mod);
}


bool CodeGen::doCodeGen(TranslationUnitAST &tunit, std::string name){
  return generateTranslationUnit(tunit, name);
}

llvm::Module &CodeGen::getModule(){
  if(Mod){
    return *Mod;
  }
  else{
    return *(new llvm::Module ("null", Context));
  }
}

bool CodeGen::generateTranslationUnit(TranslationUnitAST &tunit, std::string name){
  Mod = new llvm::Module (name, Context);
  // 構造体定義を先に処理してLLVMのStructTypeを作る
  for(int i = 0; ; i++){
    StructDeclAST *sdecl = tunit.getStruct(i);
    if(!sdecl){
      break;
    }
    registerStruct(sdecl);
  }
  // 構造体のメソッドを生成
  for(int i = 0; ; i++){
    StructDeclAST *sdecl = tunit.getStruct(i);
    if(!sdecl){
      break;
    }
    for(int j = 0; j < sdecl->getMethodNum(); j++){
      generateMethod(sdecl->getName(), sdecl->getMethod(j));
    }
  }
  for(int i = 0; ; i++){
    PrototypeAST *proto = tunit.getPrototype(i);
    if(!proto){
      break;
    }
    else if(!generatePrototype(proto, Mod)){
      SAFE_DELETE(Mod);
      return false;
    }
  }

  for(int i = 0; ; i++){
    FunctionAST *func = tunit.getFunction(i);
    if(!func){
      break;
    }
    else if(!generateFunctionDefinition(func, Mod)){
      SAFE_DELETE(Mod);
      return false;
    }
  }
  return true;
}
void CodeGen::registerStruct(StructDeclAST *struct_decl){
  // メンバの型を集める
  std::vector<llvm::Type*> member_types;
  for(int i = 0; i < struct_decl->getMemberNum(); i++){
    llvm::Type *member_type = getLLVMType(struct_decl->getMemberType(i));
    member_types.push_back(member_type);
  }
  // LLVMのStructTypeを作る（名前付き）
  llvm::StructType *struct_type = llvm::StructType::create(Context, member_types, struct_decl->getName());
  // 2つの表に登録
  StructTypeTable[struct_decl->getName()] = struct_type;
  StructInfoTable[struct_decl->getName()] = struct_decl;
}

llvm::Type *CodeGen::getLLVMType(const std::string &type_name){
  if(type_name == "int"){
    return llvm::Type::getInt32Ty(Context);
  }
  else if(type_name == "char"){
    return llvm::Type::getInt8Ty(Context);
  }
  else if(StructTypeTable.find(type_name) != StructTypeTable.end()){
    return StructTypeTable[type_name];
  }
  // 未知の型（フォールバックでint）
  return llvm::Type::getInt32Ty(Context);
}
llvm::Value *CodeGen::convertType(llvm::Value *value, llvm::Type *target_type){
  llvm::Type *src_type = value->getType();
  if(src_type == target_type){
    return value;
  }
  if(src_type->isIntegerTy() && target_type->isIntegerTy()){
    unsigned src_bits = src_type->getIntegerBitWidth();
    unsigned tgt_bits = target_type->getIntegerBitWidth();
    if(src_bits > tgt_bits){
      return Builder->CreateTrunc(value, target_type, "trunc");
    }
    else{
      return Builder->CreateSExt(value, target_type, "sext");
    }
  }
  return value;
}
llvm::Function *CodeGen::generatePrototype(PrototypeAST *proto, llvm::Module *mod){
  llvm::Function *func = mod->getFunction(proto->getName());
  if(func){
    if(func->arg_size() == proto->getParamNum() && func->empty()){
      return func;
    }
    else{
      fprintf(stderr, "error::function %s id redefined", proto->getName().c_str());
      return NULL;
    }
  }

  std::vector<llvm::Type*> int_types(proto->getParamNum(), llvm::Type::getInt32Ty(Context));
  llvm::FunctionType *func_type = llvm::FunctionType::get(llvm::Type::getInt32Ty(Context), int_types, false);
  func = llvm::Function::Create(func_type, llvm::Function::ExternalLinkage, proto->getName(), mod);

  llvm::Function::arg_iterator arg_iter = func->arg_begin();
  for(int i = 0; i< proto->getParamNum(); i++){
    arg_iter->setName(proto->getParamName(i).append("_arg"));
    arg_iter++;
  }

  return func;
}

llvm::Function *CodeGen::generateFunctionDefinition(FunctionAST *func_ast, llvm::Module *mod){
  llvm::Function *func = generatePrototype(func_ast->getPrototype(), mod);
  if(!func){
    return NULL;
  }
  CurFunc = func;
  llvm::BasicBlock *bblock = llvm::BasicBlock::Create(Context, "entry", func);
  Builder->SetInsertPoint(bblock);
  generateFunctionStatement(func_ast->getBody());

  return func;
}
void CodeGen::generateMethod(const std::string &struct_name, FunctionAST *method){
  PrototypeAST *proto = method->getPrototype();
  std::string method_name = struct_name + "_" + proto->getName();

  // this の型: 構造体へのポインタ
  llvm::StructType *struct_type = StructTypeTable[struct_name];
  llvm::PointerType *this_type = llvm::PointerType::get(struct_type, 0);

  // 関数型: int メソッド名(構造体* this)
  std::vector<llvm::Type*> arg_types;
  arg_types.push_back(this_type);
  llvm::FunctionType *func_type = llvm::FunctionType::get(llvm::Type::getInt32Ty(Context), arg_types, false);
  llvm::Function *func = llvm::Function::Create(func_type, llvm::Function::ExternalLinkage, method_name, Mod);

  // 第1引数を "this" と名付ける
  llvm::Function::arg_iterator arg_it = func->arg_begin();
  arg_it->setName("this");

  CurFunc = func;
  CurrentThis = arg_it;   // メンバ解決用に保持
  CurrentStructName = struct_name;
  llvm::BasicBlock *bblock = llvm::BasicBlock::Create(Context, "entry", func);
  Builder->SetInsertPoint(bblock);
  generateFunctionStatement(method->getBody());

  CurrentThis = NULL;
  CurrentStructName = "";
}
llvm::Value *CodeGen::generateFunctionStatement(FunctionStmtAST *func_stmt){
  VariableDeclAST *vdecl;
  llvm::Value *v = NULL;
  for(int i = 0; ; i++){
    if(!func_stmt->getVariableDecl(i)){
      break;
    }
    vdecl = llvm::dyn_cast<VariableDeclAST>(func_stmt->getVariableDecl(i));
    v = generateVariableDeclaration(vdecl);
  }

  BaseAST *stmt;
  for(int i = 0; ; i++){
    stmt = func_stmt->getStatement(i);
    if(!stmt){
      break;
    }
    else if(!llvm::isa<NullExprAST>(stmt)){
      v = generateStatement(stmt);
    }
  }

  return v;
}

llvm::Value *CodeGen::generateVariableDeclaration(VariableDeclAST *vdecl){
  llvm::Type *var_type = getLLVMType(vdecl->getTypeName());
  llvm::AllocaInst *alloca = Builder->CreateAlloca(var_type, 0, vdecl->getName());
  VariableTypeTable[vdecl->getName()] = vdecl->getTypeName();
  if(vdecl->getType() == VariableDeclAST::param){
    llvm::ValueSymbolTable *vs_table = CurFunc->getValueSymbolTable();
    Builder->CreateStore(vs_table->lookup(vdecl->getName().append("_arg")), alloca);
  }
  return alloca;
}

llvm::Value *CodeGen::generateStatement(BaseAST *stmt){
  if(llvm::isa<BinaryExprAST>(stmt)){
    return generateBinaryExprssion(llvm::dyn_cast<BinaryExprAST>(stmt));
  }
  else if(llvm::isa<CallExprAST>(stmt)){
    return generateCallExpression(llvm::dyn_cast<CallExprAST>(stmt));
  }
  else if(llvm::isa<JumpStmtAST>(stmt)){
    return generateJumpStatement(llvm::dyn_cast<JumpStmtAST>(stmt));
  }
  else if(llvm::isa<IfStmtAST>(stmt)){
    return generateIfStatement(llvm::dyn_cast<IfStmtAST>(stmt));
  }
  else if(llvm::isa<WhileStmtAST>(stmt)){
    return generateWhileStatement(llvm::dyn_cast<WhileStmtAST>(stmt));
  }
  else if(llvm::isa<ForStmtAST>(stmt)){
    return generateForStatement(llvm::dyn_cast<ForStmtAST>(stmt));
  }
  else if(llvm::isa<MemberAccessAST>(stmt)){
    return generateMethodCall(llvm::dyn_cast<MemberAccessAST>(stmt));
  }
  else{
    return NULL;
  }
}
llvm::Value *CodeGen::generateIfStatement(IfStmtAST *if_stmt){
  // 条件式を生成（i1が返る）
  BaseAST *cond = if_stmt->getCondition();
  llvm::Value *cond_v = NULL;
  if(llvm::isa<BinaryExprAST>(cond)){
    cond_v = generateBinaryExprssion(llvm::dyn_cast<BinaryExprAST>(cond));
  }
  else if(llvm::isa<VariableAST>(cond)){
    cond_v = generateVariable(llvm::dyn_cast<VariableAST>(cond));
  }
  else if(llvm::isa<NumberAST>(cond)){
    cond_v = generateNumber(llvm::dyn_cast<NumberAST>(cond)->getNumberValue());
  }
  if(!cond_v){
    return NULL;
  }

  // else節があるか判定
  bool has_else = (if_stmt->getElseStmt(0) != NULL);

  // BasicBlockを作る（elseはある場合のみ）
  llvm::BasicBlock *then_bb = llvm::BasicBlock::Create(Context, "then", CurFunc);
  llvm::BasicBlock *else_bb = NULL;
  if(has_else){
    else_bb = llvm::BasicBlock::Create(Context, "else", CurFunc);
  }
  llvm::BasicBlock *merge_bb = llvm::BasicBlock::Create(Context, "merge", CurFunc);

  // 条件で分岐: 真ならthen、偽ならelse（無ければmerge）
  if(has_else){
    Builder->CreateCondBr(cond_v, then_bb, else_bb);
  }
  else{
    Builder->CreateCondBr(cond_v, then_bb, merge_bb);
  }

  // thenブロックに命令を積んでいく
  Builder->SetInsertPoint(then_bb);
  BaseAST *stmt;
  for(int i = 0; (stmt = if_stmt->getThenStmt(i)); i++){
    generateStatement(stmt);
  }
  // thenブロックが終端を持っていなければmergeへジャンプ
  if(!Builder->GetInsertBlock()->getTerminator()){
    Builder->CreateBr(merge_bb);
  }

  // elseブロック（elseがある場合のみ）
  if(has_else){
    Builder->SetInsertPoint(else_bb);
    for(int i = 0; (stmt = if_stmt->getElseStmt(i)); i++){
      generateStatement(stmt);
    }
    // elseブロックが終端を持っていなければmergeへジャンプ
    if(!Builder->GetInsertBlock()->getTerminator()){
      Builder->CreateBr(merge_bb);
    }
  }

// mergeブロックに到達する経路があるか確認
  if(llvm::pred_begin(merge_bb) == llvm::pred_end(merge_bb)){
    // 誰も到達しない（then/elseが両方returnした）ならmergeは不要
    merge_bb->eraseFromParent();
  }
  else{
    // 到達する経路があるなら、以降の命令をmergeに積む
    Builder->SetInsertPoint(merge_bb);
  }
  return merge_bb;
}
llvm::Value *CodeGen::generateWhileStatement(WhileStmtAST *while_stmt){
  // 3つのブロックを作る
  llvm::BasicBlock *cond_bb = llvm::BasicBlock::Create(Context, "cond", CurFunc);
  llvm::BasicBlock *body_bb = llvm::BasicBlock::Create(Context, "body", CurFunc);
  llvm::BasicBlock *after_bb = llvm::BasicBlock::Create(Context, "after", CurFunc);

  // entryから条件チェックへ無条件ジャンプ
  Builder->CreateBr(cond_bb);

  // condブロック: 条件を評価して分岐
  Builder->SetInsertPoint(cond_bb);
  BaseAST *cond = while_stmt->getCondition();
  llvm::Value *cond_v = NULL;
  if(llvm::isa<BinaryExprAST>(cond)){
    cond_v = generateBinaryExprssion(llvm::dyn_cast<BinaryExprAST>(cond));
  }
  else if(llvm::isa<VariableAST>(cond)){
    cond_v = generateVariable(llvm::dyn_cast<VariableAST>(cond));
  }
  else if(llvm::isa<NumberAST>(cond)){
    cond_v = generateNumber(llvm::dyn_cast<NumberAST>(cond)->getNumberValue());
  }
  if(!cond_v){
    return NULL;
  }
  // 条件が真ならbody、偽ならafter
  Builder->CreateCondBr(cond_v, body_bb, after_bb);

  // bodyブロック: 本体を生成
  Builder->SetInsertPoint(body_bb);
  BaseAST *stmt;
  for(int i = 0; (stmt = while_stmt->getBodyStmt(i)); i++){
    generateStatement(stmt);
  }
  // bodyが終端を持っていなければ、condへ戻る（ループ）
  if(!Builder->GetInsertBlock()->getTerminator()){
    Builder->CreateBr(cond_bb);
  }

  // 以降の命令はafterブロックに積む
  Builder->SetInsertPoint(after_bb);

  return after_bb;
}
llvm::Value *CodeGen::generateForStatement(ForStmtAST *for_stmt){
  // init を1回実行
  generateStatement(for_stmt->getInit());

  // 3つのブロックを作る
  llvm::BasicBlock *cond_bb = llvm::BasicBlock::Create(Context, "for_cond", CurFunc);
  llvm::BasicBlock *body_bb = llvm::BasicBlock::Create(Context, "for_body", CurFunc);
  llvm::BasicBlock *after_bb = llvm::BasicBlock::Create(Context, "for_after", CurFunc);

  // condへ無条件ジャンプ
  Builder->CreateBr(cond_bb);

  // condブロック: 条件を評価して分岐
  Builder->SetInsertPoint(cond_bb);
  BaseAST *cond = for_stmt->getCondition();
  llvm::Value *cond_v = NULL;
  if(llvm::isa<BinaryExprAST>(cond)){
    cond_v = generateBinaryExprssion(llvm::dyn_cast<BinaryExprAST>(cond));
  }
  else if(llvm::isa<VariableAST>(cond)){
    cond_v = generateVariable(llvm::dyn_cast<VariableAST>(cond));
  }
  else if(llvm::isa<NumberAST>(cond)){
    cond_v = generateNumber(llvm::dyn_cast<NumberAST>(cond)->getNumberValue());
  }
  if(!cond_v){
    return NULL;
  }
  Builder->CreateCondBr(cond_v, body_bb, after_bb);

  // bodyブロック: 本体を生成し、その後updateを実行
  Builder->SetInsertPoint(body_bb);
  BaseAST *stmt;
  for(int i = 0; (stmt = for_stmt->getBodyStmt(i)); i++){
    generateStatement(stmt);
  }
  // bodyが終端を持っていなければ、updateを実行してcondへ戻る
  if(!Builder->GetInsertBlock()->getTerminator()){
    generateStatement(for_stmt->getUpdate());
    Builder->CreateBr(cond_bb);
  }

  // 以降の命令はafterブロックに積む
  Builder->SetInsertPoint(after_bb);

  return after_bb;
}
llvm::Value *CodeGen::generateBinaryExprssion(BinaryExprAST *bin_expr){
  BaseAST *lhs = bin_expr->getLHS();
  BaseAST *rhs = bin_expr->getRHS();
  llvm::Value *lhs_v = NULL;
  llvm::Value *rhs_v = NULL;

  if(bin_expr->getOp() == "="){
    if(llvm::isa<MemberAccessAST>(lhs)){
      lhs_v = generateMemberAddress(llvm::dyn_cast<MemberAccessAST>(lhs));
    }
    else{
      VariableAST *lhs_var = llvm::dyn_cast<VariableAST>(lhs);
      llvm::ValueSymbolTable *vs_table = CurFunc->getValueSymbolTable();
      lhs_v = vs_table->lookup(lhs_var->getName());
    }
  }
  else{
    if(llvm::isa<BinaryExprAST>(lhs)){
      lhs_v = generateBinaryExprssion(llvm::dyn_cast<BinaryExprAST>(lhs));
    }
    else if(llvm::isa<CallExprAST>(lhs)){
      lhs_v = generateCallExpression(llvm::dyn_cast<CallExprAST>(lhs));
    }
    else if(llvm::isa<VariableAST>(lhs)){
      lhs_v = generateVariable(llvm::dyn_cast<VariableAST>(lhs));
    }
    else if(llvm::isa<NumberAST>(lhs)){
      NumberAST *num = llvm::dyn_cast<NumberAST>(lhs);
      lhs_v = generateNumber(num->getNumberValue());
    }
    else if(llvm::isa<MemberAccessAST>(lhs)){
      llvm::Value *addr = generateMemberAddress(llvm::dyn_cast<MemberAccessAST>(lhs));
      lhs_v = Builder->CreateLoad(llvm::Type::getInt32Ty(Context), addr, "member_tmp");
    }
  }

  if(llvm::isa<BinaryExprAST>(rhs)){
    rhs_v = generateBinaryExprssion(llvm::dyn_cast<BinaryExprAST>(rhs));
  }
  else if(llvm::isa<CallExprAST>(rhs)){
    rhs_v = generateCallExpression(llvm::dyn_cast<CallExprAST>(rhs));
  }
  else if(llvm::isa<VariableAST>(rhs)){
    rhs_v = generateVariable(llvm::dyn_cast<VariableAST>(rhs));
  }
  else if(llvm::isa<NumberAST>(rhs)){
    NumberAST *num = llvm::dyn_cast<NumberAST>(rhs);
    rhs_v = generateNumber(num->getNumberValue());
  }
  else if(llvm::isa<MemberAccessAST>(rhs)){
    llvm::Value *addr = generateMemberAddress(llvm::dyn_cast<MemberAccessAST>(rhs));
    rhs_v = Builder->CreateLoad(llvm::Type::getInt32Ty(Context), addr, "member_tmp");
  }
  if(bin_expr->getOp() == "="){
    llvm::Type *elem_type = lhs_v->getType()->getPointerElementType();
    rhs_v = convertType(rhs_v, elem_type);
    return Builder->CreateStore(rhs_v, lhs_v);
  }
  else if(bin_expr->getOp() == "+"){
    return Builder->CreateAdd(lhs_v, rhs_v, "add_tmp");
  }
  else if(bin_expr->getOp() == "-"){
    return Builder->CreateSub(lhs_v, rhs_v, "sub_tmp");
  }
  else if(bin_expr->getOp() == "*"){
    return Builder->CreateMul(lhs_v, rhs_v, "mul_tmp");
  }
  else if(bin_expr->getOp() == "/"){
    return Builder->CreateSDiv(lhs_v, rhs_v, "div_tmp");
  }
  else if(bin_expr->getOp() == "<"){
    return Builder->CreateICmpSLT(lhs_v, rhs_v, "cmp_tmp");
  }
  else if(bin_expr->getOp() == ">"){
    return Builder->CreateICmpSGT(lhs_v, rhs_v, "cmp_tmp");
  }
  else if(bin_expr->getOp() == "<="){
    return Builder->CreateICmpSLE(lhs_v, rhs_v, "cmp_tmp");
  }
  else if(bin_expr->getOp() == ">="){
    return Builder->CreateICmpSGE(lhs_v, rhs_v, "cmp_tmp");
  }
  else if(bin_expr->getOp() == "=="){
    return Builder->CreateICmpEQ(lhs_v, rhs_v, "cmp_tmp");
  }
  else if(bin_expr->getOp() == "!="){
    return Builder->CreateICmpNE(lhs_v, rhs_v, "cmp_tmp");
  }
  return NULL;
}

llvm::Value *CodeGen::generateCallExpression(CallExprAST *call_expr){
  std::vector<llvm::Value*> arg_vec;
  BaseAST *arg;
  llvm::Value *arg_v;
  llvm::ValueSymbolTable *vs_table = CurFunc->getValueSymbolTable();

  for(int i = 0; ; i++){
    if(!(arg = call_expr->getArgs(i))){
      break;
    }

    if(llvm::isa<CallExprAST>(arg)){
      arg_v = generateCallExpression(llvm::dyn_cast<CallExprAST>(arg));
    }
    else if(llvm::isa<BinaryExprAST>(arg)){
      BinaryExprAST *bin_expr = llvm::dyn_cast<BinaryExprAST>(arg);
      arg_v = generateBinaryExprssion(llvm::dyn_cast<BinaryExprAST>(arg));
      if(bin_expr->getOp() == "="){
        VariableAST *var = llvm::dyn_cast<VariableAST>(bin_expr->getLHS());
        llvm::Value *ptr = vs_table->lookup(var->getName());
        arg_v = Builder->CreateLoad(llvm::Type::getInt32Ty(Context), ptr, "arg_val");
      }
    }
    else if(llvm::isa<VariableAST>(arg)){
      arg_v = generateVariable(llvm::dyn_cast<VariableAST>(arg));
    }
    else if(llvm::isa<NumberAST>(arg)){
      NumberAST *num = llvm::dyn_cast<NumberAST>(arg);
      arg_v = generateNumber(num->getNumberValue());
    }
    arg_vec.push_back(arg_v);
  }

  return Builder->CreateCall(Mod->getFunction(call_expr->getCallee()), arg_vec, "call_tmp");
}

llvm::Value *CodeGen::generateJumpStatement(JumpStmtAST *jump_stmt){
  BaseAST *expr = jump_stmt->getExpr();
  llvm::Value *ret_v = NULL;

  if(llvm::isa<BinaryExprAST>(expr)){
    ret_v = generateBinaryExprssion(llvm::dyn_cast<BinaryExprAST>(expr));
  }
  else if(llvm::isa<VariableAST>(expr)){
    ret_v = generateVariable(llvm::dyn_cast<VariableAST>(expr));
  }
  else if(llvm::isa<NumberAST>(expr)){
    NumberAST *num = llvm::dyn_cast<NumberAST>(expr);
    ret_v = generateNumber(num->getNumberValue());
  }
  else if(llvm::isa<CallExprAST>(expr)){
    ret_v = generateCallExpression(llvm::dyn_cast<CallExprAST>(expr));
  }
  else if(llvm::isa<MemberAccessAST>(expr)){
    llvm::Value *addr = generateMemberAddress(llvm::dyn_cast<MemberAccessAST>(expr));
    ret_v = Builder->CreateLoad(llvm::Type::getInt32Ty(Context), addr, "member_tmp");
  }
  DBG("[CG] JumpStmt: exprType binary=%d var=%d num=%d, ret_v=%p\n",
          llvm::isa<BinaryExprAST>(expr), llvm::isa<VariableAST>(expr),
          llvm::isa<NumberAST>(expr), (void*)ret_v);
  
  ret_v = convertType(ret_v, CurFunc->getReturnType());
  return Builder->CreateRet(ret_v);
}

llvm::Value *CodeGen::generateVariable(VariableAST *var){
  llvm::ValueSymbolTable *vs_table = CurFunc->getValueSymbolTable();
  llvm::Value *ptr = vs_table->lookup(var->getName());
  llvm::Type *var_type = getLLVMType(VariableTypeTable[var->getName()]);
  return Builder->CreateLoad(var_type, ptr, "var_tmp");
}
llvm::Value *CodeGen::generateMemberAddress(MemberAccessAST *member){
  llvm::Value *base_ptr;
  std::string type_name;
  if(member->getVariableName() == "this"){
    // メソッド内: this は第1引数、型は現在の構造体
    base_ptr = CurrentThis;
    type_name = CurrentStructName;
  }
  else{
    // 通常の変数 p
    llvm::ValueSymbolTable *vs_table = CurFunc->getValueSymbolTable();
    base_ptr = vs_table->lookup(member->getVariableName());
    type_name = VariableTypeTable[member->getVariableName()];
  }

  // その構造体の情報を得る
  StructDeclAST *struct_decl = StructInfoTable[type_name];
  llvm::StructType *struct_type = StructTypeTable[type_name];

  // メンバ x が何番目かを探す
  int member_index = -1;
  for(int i = 0; i < struct_decl->getMemberNum(); i++){
    if(struct_decl->getMemberName(i) == member->getMemberName()){
      member_index = i;
      break;
    }
  }
  if(member_index < 0){
    return NULL;
  }

  // getelementptr で「p の member_index 番目のフィールド」のアドレスを計算
  return Builder->CreateStructGEP(struct_type, base_ptr, member_index, "member_ptr");
}
llvm::Value *CodeGen::generateMethodCall(MemberAccessAST *member){
  llvm::ValueSymbolTable *vs_table = CurFunc->getValueSymbolTable();
  llvm::Value *this_ptr = vs_table->lookup(member->getVariableName());
  std::string type_name = VariableTypeTable[member->getVariableName()];
  std::string method_name = type_name + "_" + member->getMemberName();
  llvm::Function *method = Mod->getFunction(method_name);
  std::vector<llvm::Value*> args;
  args.push_back(this_ptr);
  return Builder->CreateCall(method, args, "method_call");
}
llvm::Value *CodeGen::generateNumber(int value){
  return llvm::ConstantInt::get(llvm::Type::getInt32Ty(Context), value);
}