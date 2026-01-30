#pragma once

#include <VCL/Core/Identifier.hpp>
#include <VCL/AST/Decl.hpp>

#include <llvm/ADT/DenseMap.h>
#include <llvm/ADT/IntrusiveRefCntPtr.h>


namespace VCL {

    class SymbolTable : public llvm::RefCountedBase<SymbolTable> {
    public:
        SymbolTable() = default;
        SymbolTable(const SymbolTable& other) = delete;
        SymbolTable(SymbolTable&& other) = delete;
        ~SymbolTable() = default;

        SymbolTable& operator=(const SymbolTable& other) = delete;
        SymbolTable& operator=(SymbolTable&& other) = delete;

        inline bool Add(IdentifierInfo* identifierInfo, Decl* decl) {
            if (symbols.count(identifierInfo))
                return false;
            symbols.insert({ identifierInfo, decl });
            return true;
        }

        inline Decl* Get(IdentifierInfo* identifierInfo) {
            if (!symbols.count(identifierInfo))
                return nullptr;
            return symbols.at(identifierInfo);
        }

        inline llvm::DenseMap<IdentifierInfo*, Decl*>::const_iterator begin() const { return symbols.begin(); }
        inline llvm::DenseMap<IdentifierInfo*, Decl*>::const_iterator end() const { return symbols.end(); }

    private:
        llvm::DenseMap<IdentifierInfo*, Decl*> symbols{};
    };

}