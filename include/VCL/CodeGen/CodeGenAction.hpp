#pragma once

#include <llvm/ExecutionEngine/Orc/ThreadSafeModule.h>


namespace VCL {

    class CodeGenAction {
    public:
        CodeGenAction() = default;
        CodeGenAction(const CodeGenAction& other) = delete;
        CodeGenAction(CodeGenAction&& other) = delete;
        virtual ~CodeGenAction() = default;
        
        CodeGenAction& operator=(const CodeGenAction& other) = delete;
        CodeGenAction& operator=(CodeGenAction&& other) = delete;

        inline llvm::orc::ThreadSafeModule& GetModule() { return module; }
        inline llvm::orc::ThreadSafeModule MoveModule() { return std::move(module); }
    
    protected:
        llvm::orc::ThreadSafeModule module;
    };

}