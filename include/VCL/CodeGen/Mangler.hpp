#pragma once

#include <VCL/AST/ASTContext.hpp>
#include <VCL/AST/Decl.hpp>

#include <string>


namespace VCL {

    /**
     * Symbol names: `<ASTContext prefix>.<name>[.<n>]` (see ASTContext::GetMangledName), except for
     * `in`/`out` globals, which keep their name so the host can bind or look them up.
     * `context` is the ASTContext the declaration belongs to.
     */
    class Mangler {
    public:
        static std::string MangleFunctionDecl(ASTContext& context, FunctionDecl* decl);
        static std::string MangleVarDecl(ASTContext& context, VarDecl* decl);
    };

}
