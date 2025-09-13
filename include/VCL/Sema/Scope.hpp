#pragma once

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


        inline Scope* GetParentScope() const { return parent; }
        inline Scope* GetBreakScope() const { return breakParent; }
        inline Scope* GetContinueScope() const { return continueParent; }
        
        inline DeclContext* GetDeclContext() { return context; }

    private:
        Scope* parent = nullptr;
        Scope* breakParent = nullptr;
        Scope* continueParent = nullptr;
        DeclContext* context = nullptr;
    };

}