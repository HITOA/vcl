#pragma once

#include <VCL/Core/Target.hpp>
#include <VCL/Core/Diagnostic.hpp>
#include <VCL/AST/ASTContext.hpp>
#include <VCL/AST/DeclContext.hpp>
#include <VCL/AST/Decl.hpp>
#include <VCL/AST/Expr.hpp>
#include <VCL/AST/ConstantValue.hpp>
#include <VCL/Sema/ModuleTable.hpp>
#include <VCL/CodeGen/CodeGenTypes.hpp>
#include <VCL/CodeGen/CodeGenFunction.hpp>

#include <llvm/ExecutionEngine/Orc/ThreadSafeModule.h>
#include <llvm/ADT/DenseMap.h>
#include <llvm/IR/GlobalValue.h>
#include <llvm/IR/Constant.h>


namespace VCL {

    class CodeGenModule {
    public:
        CodeGenModule() = delete;
        CodeGenModule(llvm::Module& module, ASTContext& ast, DiagnosticReporter& diagnosticReporter, Target& target, 
            ModuleTable& importedModules, AttributeTable& attributeTable, IdentifierTable& identifierTable);
        CodeGenModule(const CodeGenModule& other) = delete;
        CodeGenModule(CodeGenModule&& other) = default;
        ~CodeGenModule() = default;

        CodeGenModule& operator=(const CodeGenModule& other) = delete;
        CodeGenModule& operator=(CodeGenModule&& other) = default;
        
        inline ASTContext& GetASTContext() { return astContext; }
        inline DiagnosticReporter& GetDiagnosticReporter() { return diagnosticReporter; }
        inline llvm::LLVMContext& GetLLVMContext() { return module.getContext(); }
        inline llvm::Module& GetLLVMModule() { return module; }
        inline Target& GetTarget() { return target; }
        inline CodeGenTypes& GetCGT() { return cgt; }
        inline AttributeTable& GetAttributeTable() { return attributeTable; }
        inline IdentifierTable& GetIdentifierTable() { return identifierTable; }

        bool LinkNow();

        bool Emit();
        bool EmitTopLevelDecl(Decl* decl);
        bool EmitGlobalVarDecl(VarDecl* decl, bool imported = false);
        bool EmitFunctionDecl(FunctionDecl* decl, bool imported = false);
        bool EmitTemplateDecl(TemplateDecl* decl);

        bool IsDeclImported(Decl* decl);
        Module* GetImportedDeclModule(Decl* decl);
        bool EmitImportedDecl(Decl* decl);

        llvm::GlobalValue* GetGlobalDeclValue(Decl* decl);

        // CGConstantValue

        llvm::Constant* GenerateConstantValue(ConstantValue* value);
        llvm::Constant* GenerateConstantScalar(ConstantScalar* scalar);
        llvm::Constant* GenerateConstantAggregate(ConstantAggregate* aggregate);
        llvm::Constant* GenerateConstantNull(ConstantNull* null);
        
    private:
        llvm::Module& module;
        ASTContext& astContext;
        DiagnosticReporter& diagnosticReporter;
        Target& target;
        ModuleTable& importedModules;
        AttributeTable& attributeTable;
        IdentifierTable& identifierTable;

        llvm::DenseMap<Decl*, llvm::GlobalValue*> globals;
        
        CodeGenTypes cgt;
    };

}