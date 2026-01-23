#pragma once

#include <VCL/Core/Source.hpp>
#include <VCL/Sema/SymbolTable.hpp>
#include <VCL/Frontend/CompilerInstance.hpp>

#include <llvm/IR/Module.h>
#include <llvm/ExecutionEngine/Orc/ThreadSafeModule.h>
#include <llvm/ADT/IntrusiveRefCntPtr.h>
#include <llvm/ADT/DenseMap.h>


namespace VCL {

    class Module {
    public:
        Module() = delete;
        Module(std::shared_ptr<CompilerInstance> instance, llvm::orc::ThreadSafeModule&& module) 
            : instance{ instance }, module{ std::move(module) } {}
        Module(const Module& other) = default;
        Module(Module&& other) = default;
        ~Module() = default;

        Module& operator=(const Module& other) = default;
        Module& operator=(Module&& other) = default;

        inline std::shared_ptr<CompilerInstance> GetCompilerInstance() { return instance; }
    
        inline llvm::orc::ThreadSafeModule& GetModule() { return module; }
        inline llvm::orc::ThreadSafeModule MoveModule() { return std::move(module); }

    private:
        std::shared_ptr<CompilerInstance> instance = nullptr;
        llvm::orc::ThreadSafeModule module;
    };

    class ModuleCache : public llvm::RefCountedBase<ModuleCache> {
    public:
        ModuleCache() = default;
        ModuleCache(const ModuleCache& other) = delete;
        ModuleCache(ModuleCache&& other) = delete;
        ~ModuleCache() = default;

        ModuleCache& operator=(const ModuleCache& other) = delete;
        ModuleCache& operator=(ModuleCache&& other) = delete;

        inline Module* Get(Source* source) {
            if (modules.count(source->GetBufferIdentifier()))
                return modules[source->GetBufferIdentifier()];
            return nullptr;
        }

        inline Module* Add(Source* source, std::shared_ptr<CompilerInstance> instance, llvm::orc::ThreadSafeModule&& module) {
            if (modules.count(source->GetBufferIdentifier()))
                return nullptr;
            Module* m = modules.getAllocator().Allocate<Module>();
            new (m) Module{ instance, std::move(module) };
            modules.insert({ source->GetBufferIdentifier(), m });
            return m;
        }

    private:
        llvm::StringMap<Module*> modules{};
    };

}