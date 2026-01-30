#pragma once

#include <VCL/Core/Directive.hpp>


namespace VCL {
    class CompilerContext;

    class ImportDirective : public DirectiveHandler {
    public:
        ImportDirective() = delete;
        ImportDirective(CompilerContext& compilerContext, const std::string& importRootDirectory) 
                : compilerContext{ compilerContext }, importRootDirectory{ importRootDirectory } {}
        ~ImportDirective() = default;

        bool OnSema(Sema& sema, DirectiveDecl* decl) override;

    private:
        CompilerContext& compilerContext;
        std::string importRootDirectory;
    };

}