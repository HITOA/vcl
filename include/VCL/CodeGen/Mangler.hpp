#pragma once

#include <VCL/AST/ASTContext.hpp>
#include <VCL/AST/Decl.hpp>

#include <string>


namespace VCL {

    class Mangler {
    public:
        static std::string MangleFunctionDecl(ASTContext& context, FunctionDecl* decl);
        static std::string MangleVarDecl(ASTContext& context, VarDecl* decl);

    private:
        static uint64_t HashType(QualType type);
    };

}