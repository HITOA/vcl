#pragma once

#include <VCL/Sema/Scope.hpp>

#include <llvm/Support/Allocator.h>
#include <llvm/ADT/DenseMap.h>


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
            Scope* scope = nullptr;
            if (context && declContextScope.count(context)) {
                scope = declContextScope.at(context);
                scope->SetParentScope(currentFrontScope);
            } else if (context) {
                void* ptr = allocator.Allocate(sizeof(Scope), 4);
                scope = new (ptr) Scope{ currentFrontScope, context };
                declContextScope.insert({ context, scope });
            } else {
                void* ptr = allocator.Allocate(sizeof(Scope), 4);
                scope = new (ptr) Scope{ currentFrontScope, context };
            }
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
        llvm::DenseMap<DeclContext*, Scope*> declContextScope{};
    };

}