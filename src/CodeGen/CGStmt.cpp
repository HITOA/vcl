#include <VCL/CodeGen/CodeGenFunction.hpp>

#include <VCL/CodeGen/CodeGenModule.hpp>


bool VCL::CodeGenFunction::GenerateStmt(Stmt* stmt) {
    switch (stmt->GetStmtClass()) {
        case Stmt::ValueStmtClass: return GenerateValueStmt((ValueStmt*)stmt);
        case Stmt::DeclStmtClass: return GenerateDeclStmt((DeclStmt*)stmt);
        case Stmt::CompoundStmtClass: return GenerateCompoundStmt((CompoundStmt*)stmt);
        case Stmt::ReturnStmtClass: return GenerateReturnStmt((ReturnStmt*)stmt);
        case Stmt::IfStmtClass: return GenerateIfStmt((IfStmt*)stmt);
        case Stmt::WhileStmtClass: return GenerateWhileStmt((WhileStmt*)stmt);
        case Stmt::ForStmtClass: return GenerateForStmt((ForStmt*)stmt);
        case Stmt::BreakStmtClass: return GenerateBreakStmt((BreakStmt*)stmt);
        case Stmt::ContinueStmtClass: return GenerateContinueStmt((ContinueStmt*)stmt);
        default:
            cgm.GetDiagnosticReporter().Error(Diagnostic::MissingImplementation)
                .SetCompilerInfo(__FILE__, __func__, __LINE__)
                .Report();
            return false;
    }
}

bool VCL::CodeGenFunction::GenerateValueStmt(ValueStmt* stmt) {
    llvm::Value* value = GenerateExpr((Expr*)stmt);
    return value != nullptr;
}

bool VCL::CodeGenFunction::GenerateDeclStmt(DeclStmt* stmt) {
    Decl* decl = stmt->GetDecl();
    return GenerateDecl(decl);
}

bool VCL::CodeGenFunction::GenerateCompoundStmt(CompoundStmt* stmt) {
    for (Stmt* child : stmt->GetStmts()) {
        if (!GenerateStmt(child))
            return false;
    }
    return true;
}

bool VCL::CodeGenFunction::GenerateReturnStmt(ReturnStmt* stmt) {
    if (stmt->GetExpr()) {
        llvm::Value* value = GenerateExpr(stmt->GetExpr());
        if (!value)
            return false;
        builder.CreateRet(value);
    } else {
        builder.CreateRetVoid();
    }
    return true;
}

bool VCL::CodeGenFunction::GenerateIfStmt(IfStmt* stmt) {
    llvm::Value* condition = GenerateExpr(stmt->GetCondition());
    if (!condition)
        return false;
    
    llvm::BasicBlock* thenBB = llvm::BasicBlock::Create(cgm.GetLLVMContext(), "then", function);
    llvm::BasicBlock* elseBB = llvm::BasicBlock::Create(cgm.GetLLVMContext(), "else", function);
    llvm::BasicBlock* endBB = llvm::BasicBlock::Create(cgm.GetLLVMContext(), "end", function);

    builder.CreateCondBr(condition, thenBB, elseBB);
    builder.SetInsertPoint(thenBB);
    if (!GenerateStmt(stmt->GetThenStmt()))
        return false;
    if (!builder.GetInsertBlock()->getTerminator())
        builder.CreateBr(endBB);

    builder.SetInsertPoint(elseBB);
    if (stmt->GetElseStmt())
        if (!GenerateStmt(stmt->GetElseStmt()))
            return false;
    if (!builder.GetInsertBlock()->getTerminator())
        builder.CreateBr(endBB);
    
    builder.SetInsertPoint(endBB);
    return true;
}

bool VCL::CodeGenFunction::GenerateWhileStmt(WhileStmt* stmt) {
    llvm::BasicBlock* conditionBB = llvm::BasicBlock::Create(cgm.GetLLVMContext(), "while", function);
    llvm::BasicBlock* thenBB = llvm::BasicBlock::Create(cgm.GetLLVMContext(), "loop", function);
    llvm::BasicBlock* endBB = llvm::BasicBlock::Create(cgm.GetLLVMContext(), "end", function);

    builder.CreateBr(conditionBB);
    builder.SetInsertPoint(conditionBB);
    
    llvm::Value* condition = GenerateExpr(stmt->GetCondition());
    if (!condition)
        return false;

    builder.CreateCondBr(condition, thenBB, endBB);

    builder.SetInsertPoint(thenBB);

    PushBreakBB(endBB);
    PushContinueBB(conditionBB);
    bool s = GenerateStmt(stmt->GetThenStmt());
    PopContinueBB();
    PopBreakBB();
    if (!s)
        return false;

    if (!builder.GetInsertBlock()->getTerminator())
        builder.CreateBr(conditionBB);

    builder.SetInsertPoint(endBB);
    return true;
}

bool VCL::CodeGenFunction::GenerateForStmt(ForStmt* stmt) {
    if (stmt->GetStartStmt())
        if (!GenerateStmt(stmt->GetStartStmt()))
            return false;

    llvm::BasicBlock* conditionBB = llvm::BasicBlock::Create(cgm.GetLLVMContext(), "for", function);
    llvm::BasicBlock* thenBB = llvm::BasicBlock::Create(cgm.GetLLVMContext(), "loop", function);
    llvm::BasicBlock* loopExprBB = llvm::BasicBlock::Create(cgm.GetLLVMContext(), "loop_expr", function);
    llvm::BasicBlock* endBB = llvm::BasicBlock::Create(cgm.GetLLVMContext(), "end", function);

    builder.CreateBr(conditionBB);
    builder.SetInsertPoint(conditionBB);

    llvm::Value* condition = llvm::ConstantInt::get(llvm::IntegerType::get(cgm.GetLLVMContext(), 1), 1);
    if (stmt->GetCondition()) {
        condition = GenerateExpr(stmt->GetCondition());
        if (!condition)
            return false;
    }

    builder.CreateCondBr(condition, thenBB, endBB);

    builder.SetInsertPoint(thenBB);

    PushBreakBB(endBB);
    PushContinueBB(loopExprBB);
    bool s = GenerateStmt(stmt->GetThenStmt());
    PopContinueBB();
    PopBreakBB();
    if (!s)
        return false;

    if (!builder.GetInsertBlock()->getTerminator()) {
        builder.CreateBr(loopExprBB);
        builder.SetInsertPoint(loopExprBB);
        if (stmt->GetLoopExpr())
            if (!GenerateExpr(stmt->GetLoopExpr()))
                return false;
        builder.CreateBr(conditionBB);
    }

    builder.SetInsertPoint(endBB);
    
    return true;
}

bool VCL::CodeGenFunction::GenerateBreakStmt(BreakStmt* stmt) {
    llvm::BasicBlock* breakBB = breakBBStack.back();
    builder.CreateBr(breakBB);
    return true;
}

bool VCL::CodeGenFunction::GenerateContinueStmt(ContinueStmt* stmt) {
    llvm::BasicBlock* continueBB = continueBBStack.back();
    builder.CreateBr(continueBB);
    return true;
}