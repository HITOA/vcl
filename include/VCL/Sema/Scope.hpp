#pragma once

#include <VCL/Core/Identifier.hpp>
#include <VCL/AST/Decl.hpp>

#include <llvm/ADT/SmallPtrSet.h>


namespace VCL {

    class Scope {
    public:
        Scope() = default;
        Scope(Scope* parent, DeclContext* context) : parent{ parent }, context{ context } {}
        Scope(const Scope& other) = delete;
        Scope(Scope&& other) = delete;
        ~Scope() = default;

        Scope& operator=(const Scope& other) = delete;
        Scope& operator=(Scope&& other) = delete;

        inline void SetParentScope(Scope* parent) { this->parent = parent; }
        inline void SetBreakScope(Scope* breakParent) { this->breakParent = breakParent; }
        inline void SetContinueScope(Scope* continueParent) { this->continueParent = continueParent; }

        inline Scope* GetParentScope() const { return parent; }
        inline Scope* GetBreakScope() const { return breakParent; }
        inline Scope* GetContinueScope() const { return continueParent; }
        
        inline DeclContext* GetDeclContext() { return context; }

        inline bool AddDeclInScope(Decl* decl) { return declsInScope.insert(decl).second; }
        inline llvm::SmallPtrSetIterator<Decl*> begin() { return declsInScope.begin(); }
        inline llvm::SmallPtrSetIterator<Decl*> end() { return declsInScope.end(); }

    private:
        Scope* parent = nullptr;
        Scope* breakParent = nullptr;
        Scope* continueParent = nullptr;
        DeclContext* context = nullptr;
        llvm::SmallPtrSet<Decl*, 32> declsInScope{};
    };

}