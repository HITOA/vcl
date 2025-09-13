#pragma once

#include <VCL/Sema/Scope.hpp>

#include <llvm/Support/Allocator.h>


namespace VCL {

    class ScopeManager {
    public:
        ScopeManager() = default;
        ScopeManager(const ScopeManager& other) = delete;
        ScopeManager(ScopeManager&& other) = delete;
        ~ScopeManager() = default;

        ScopeManager& operator=(const ScopeManager& other) = delete;
        ScopeManager& operator=(ScopeManager&& other) = delete;

        inline Scope* EmplaceScopeFront(DeclContext* context) {
            void* ptr = allocator.Allocate(sizeof(Scope), 4);
            Scope* scope = new (ptr) Scope{ currentFrontScope, context };
            currentFrontScope = scope;
            return scope;
        }

        inline void PopScopeFront(DeclContext* context) {
            currentFrontScope = currentFrontScope->GetParentScope();
        }

        inline Scope* GetScopeFront() { return currentFrontScope; }
    
    private:
        llvm::BumpPtrAllocator allocator{};
        Scope* currentFrontScope = nullptr;
    };

}