#include <VCL/CodeGen/CodeGenFunction.hpp>

#include <VCL/CodeGen/CodeGenModule.hpp>



bool VCL::CodeGenFunction::GenerateDecl(Decl* decl) {
    switch (decl->GetDeclClass()) {
        case Decl::VarDeclClass:
            return GenerateVarDecl((VarDecl*)decl);
        default:
            cgm.GetDiagnosticReporter().Error(Diagnostic::InternalError)
                .SetCompilerInfo(__FILE__, __func__, __LINE__)
                .Report();
            return false;
    }
}

bool VCL::CodeGenFunction::GenerateVarDecl(VarDecl* decl) {
    llvm::Type* type = cgm.GetCGT().ConvertType(decl->GetValueType());
    llvm::AllocaInst* alloca = GenerateAllocaInst(type, decl->GetIdentifierInfo()->GetName());
    Expr* initializer = decl->GetInitializer();
    if (initializer) {
        llvm::Value* initializerValue = GenerateExpr(initializer);
        if (!initializerValue)
            return false;
        builder.CreateStore(initializerValue, alloca);
    } else {
        builder.CreateStore(llvm::Constant::getNullValue(type), alloca);
    }

    if (Type::GetCanonicalType(decl->GetValueType().GetType())->GetTypeClass() == Type::LanesTypeClass) {
        llvm::Align align{ (uint64_t)cgm.GetTarget().GetVectorWidthInByte() };
        alloca->setAlignment(align);
    }
    
    locals.insert(std::make_pair(decl, alloca));
    return true;
}