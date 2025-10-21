#include <VCL/CodeGen/CodeGenFunction.hpp>

#include <VCL/CodeGen/CodeGenModule.hpp>


bool VCL::CodeGenFunction::GenerateStmt(Stmt* stmt) {
    switch (stmt->GetStmtClass()) {
        case Stmt::ValueStmtClass: return GenerateValueStmt((ValueStmt*)stmt);
        case Stmt::DeclStmtClass: return GenerateDeclStmt((DeclStmt*)stmt);
        case Stmt::CompoundStmtClass: return GenerateCompoundStmt((CompoundStmt*)stmt);
        case Stmt::ReturnStmtClass: return GenerateReturnStmt((ReturnStmt*)stmt);
        default:
            cgm.GetCC().GetDiagnosticReporter().Error(Diagnostic::MissingImplementation)
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
    llvm::Value* value = GenerateRValueExpr(stmt->GetExpr());
    if (!value)
        return false;
    builder.CreateRet(value);
    return true;
}