#pragma once

#include <VCL/Core/Target.hpp>
#include <VCL/Core/CompilerContext.hpp>
#include <VCL/AST/ASTContext.hpp>
#include <VCL/AST/DeclContext.hpp>
#include <VCL/AST/Decl.hpp>
#include <VCL/CodeGen/CodeGenTypes.hpp>

#include <llvm/ExecutionEngine/Orc/ThreadSafeModule.h>


namespace VCL {

    class CodeGenModule {
    public:
        CodeGenModule() = delete;
        CodeGenModule(llvm::orc::ThreadSafeModule& module, ASTContext& ast, CompilerContext& cc, Target& target);
        CodeGenModule(const CodeGenModule& other) = delete;
        CodeGenModule(CodeGenModule&& other) = default;
        ~CodeGenModule() = default;

        CodeGenModule& operator=(const CodeGenModule& other) = delete;
        CodeGenModule& operator=(CodeGenModule&& other) = default;

        inline ASTContext& GetASTContext() { return astContext; }
        inline CompilerContext& GetCC() { return cc; }
        inline llvm::LLVMContext& GetLLVMContext() { return *module.getContext().getContext(); }
        inline llvm::Module& GetLLVMModule() { return *module.getModuleUnlocked(); }
        inline Target& GetTarget() { return target; }

        bool Emit();
        bool EmitTopLevelDecl(Decl* decl);
        bool EmitGlobalVarDecl(VarDecl* decl);

    private:
        llvm::orc::ThreadSafeModule& module;
        ASTContext& astContext;
        CompilerContext& cc;
        Target& target;
        
        CodeGenTypes cgt;
    };

}