#pragma once

#include <llvm/ADT/IntrusiveRefCntPtr.h>
#include <llvm/ADT/StringRef.h>

#include <string>
#include <optional>


namespace VCL {
    class CompilerContext;
    class Source;
    class ASTContext;
    class FrontendAction;
    class SymbolTable;
    class ModuleTable;
    class DefineTable;

    class CompilerInstance {
    public:
        CompilerInstance() = delete;
        CompilerInstance(CompilerContext& compilerCtx);
        CompilerInstance(const CompilerInstance& other) = delete;
        CompilerInstance(CompilerInstance&& other) = delete;
        ~CompilerInstance();

        CompilerInstance& operator=(const CompilerInstance& other) = delete;
        CompilerInstance& operator=(CompilerInstance&& other) = delete;

        CompilerContext& GetCompilerContext();

        ASTContext& GetASTContext();
        bool HasASTContext();
        void CreateASTContext();

        SymbolTable& GetExportSymbolTable();
        bool HasExportSymbolTable();
        void CreateExportSymbolTable();

        ModuleTable& GetImportModuleTable();
        bool HasImportModuleTable();
        void CreateImportModuleTable();

        DefineTable& GetDefineTable();
        bool HasDefineTable();
        void CreateDefineTable();

        bool BeginSource(Source* source);
        bool EndSource();
        bool HasSource();
        Source* GetSource();
        
        bool ExecuteAction(FrontendAction& action);

        std::optional<std::string> GetMangledSymbolName(llvm::StringRef name);

    private:
        CompilerContext& compilerCtx;
        Source* source;
        llvm::IntrusiveRefCntPtr<ASTContext> astCtx;
        llvm::IntrusiveRefCntPtr<SymbolTable> exportedSymbols;
        llvm::IntrusiveRefCntPtr<ModuleTable> importedModules;
        llvm::IntrusiveRefCntPtr<DefineTable> definedValues;
    };

}