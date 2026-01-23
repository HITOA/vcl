#pragma once

#include <VCL/Core/Identifier.hpp>


namespace VCL {

    class SymbolRef {
    public:
        SymbolRef() = delete;
        SymbolRef(IdentifierInfo* symbolName) : moduleName{ nullptr }, symbolName{ symbolName } {}
        SymbolRef(IdentifierInfo* moduleName, IdentifierInfo* symbolName) : moduleName{ moduleName }, symbolName{ symbolName } {}
        SymbolRef(const SymbolRef& other) = default;
        SymbolRef(SymbolRef&& other) = default;
        ~SymbolRef() = default;
        
        SymbolRef& operator=(const SymbolRef& other) = default;
        SymbolRef& operator=(SymbolRef&& other) = default;

        inline bool IsLocal() { return moduleName == nullptr; }

        inline IdentifierInfo* GetModuleName() { return moduleName; }
        inline IdentifierInfo* GetSymbolName() { return symbolName; }

    private:
        IdentifierInfo* moduleName;
        IdentifierInfo* symbolName;
    };

}