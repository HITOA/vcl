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

        // Drop the cached module for a buffer identifier, releasing its LLVM module and the
        // CompilerInstance that produced it. Returns true if an entry was removed. The next
        // Get() for that identifier misses, forcing a recompile against the current source.
        inline bool Invalidate(llvm::StringRef bufferIdentifier) {
            auto it = modules.find(bufferIdentifier);
            if (it == modules.end())
                return false;
            it->second->~Module();
            modules.getAllocator().Deallocate(it->second);
            modules.erase(it);
            return true;
        }

        inline bool Invalidate(Source* source) {
            return Invalidate(source->GetBufferIdentifier());
        }

    private:
        llvm::StringMap<Module*> modules{};
    };

}