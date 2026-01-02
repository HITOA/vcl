#include <VCL/CodeGen/CodeGenFunction.hpp>

#include <VCL/CodeGen/CodeGenModule.hpp>


bool VCL::CodeGenFunction::GenerateStmt(Stmt* stmt) {
    switch (stmt->GetStmtClass()) {
        case Stmt::ValueStmtClass: return GenerateValueStmt((ValueStmt*)stmt);
        case Stmt::DeclStmtClass: return GenerateDeclStmt((DeclStmt*)stmt);
        case Stmt::CompoundStmtClass: return GenerateCompoundStmt((CompoundStmt*)stmt);
        case Stmt::ReturnStmtClass: return GenerateReturnStmt((ReturnStmt*)stmt);
        case Stmt::IfStmtClass: return GenerateIfStmt((IfStmt*)stmt);
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