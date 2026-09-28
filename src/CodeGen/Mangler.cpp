#include <VCL/CodeGen/Mangler.hpp>


std::string VCL::Mangler::MangleFunctionDecl(ASTContext& context, FunctionDecl* decl) {
    return context.GetMangledName(decl);
}

std::string VCL::Mangler::MangleVarDecl(ASTContext& context, VarDecl* decl) {
    if (decl->HasInAttribute() || decl->HasOutAttribute())
        return decl->GetIdentifierInfo()->GetName().str();
    return context.GetMangledName(decl);
}
