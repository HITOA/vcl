#pragma once

#include <vector>

#include <VCL/Sema/Scope.hpp>

#include <llvm/Support/Allocator.h>
#include <llvm/ADT/DenseMap.h>


namespace VCL {

    class ScopeManager {
    public:
        ScopeManager() = default;
        ScopeManager(const ScopeManager& other) = delete;
        ScopeManager(ScopeManager&& other) = delete;
        ~ScopeManager() {
            // The bump allocator frees memory without running destructors; a Scope's decl set
            // allocates on the heap once it grows.
            for (Scope* scope : scopes)
                scope->~Scope();
        }

        ScopeManager& operator=(const ScopeManager& other) = delete;
        ScopeManager& operator=(ScopeManager&& other) = delete;

        inline Scope* EmplaceScopeFront(DeclContext* context) {
            Scope* scope = nullptr;
            if (context && declContextScope.count(context)) {
                scope = declContextScope.at(context);
                scope->SetParentScope(currentFrontScope);
            } else if (context) {
                scope = CreateScope(context);
                declContextScope.insert({ context, scope });
            } else {
                scope = CreateScope(context);
            }
            currentFrontScope = scope;
            return scope;
        }

        inline void PopScopeFront(DeclContext* context) {
            currentFrontScope = currentFrontScope->GetParentScope();
        }

        inline Scope* GetScopeFront() { return currentFrontScope; }
        inline void SetScopeFront(Scope* scope) { currentFrontScope = scope; }

        /**
         * Makes `context`'s scope the front scope, with no parent: nothing declared outside it is
         * visible from it. Returns the previous front scope, to restore with SetScopeFront.
         */
        inline Scope* EmplaceRootScopeFront(DeclContext* context) {
            Scope* previous = currentFrontScope;
            currentFrontScope = nullptr;
            EmplaceScopeFront(context);
            return previous;
        }
    
    private:
        inline Scope* CreateScope(DeclContext* context) {
            void* ptr = allocator.Allocate(sizeof(Scope), alignof(Scope));
            Scope* scope = new (ptr) Scope{ currentFrontScope, context };
            scopes.push_back(scope);
            return scope;
        }

        llvm::BumpPtrAllocator allocator{};
        std::vector<Scope*> scopes{};
        Scope* currentFrontScope = nullptr;
        llvm::DenseMap<DeclContext*, Scope*> declContextScope{};
    };

}