#pragma once

#include <VCL/Core/Identifier.hpp>
#include <VCL/Frontend/ModuleCache.hpp>

#include <llvm/ADT/DenseMap.h>


namespace VCL {

    class ModuleTable : public llvm::RefCountedBase<ModuleTable> {
    public:
        ModuleTable() = default;
        ModuleTable(const ModuleTable& other) = delete;
        ModuleTable(ModuleTable&& other) = delete;
        ~ModuleTable() = default;

        ModuleTable& operator=(const ModuleTable& other) = delete;
        ModuleTable& operator=(ModuleTable&& other) = delete;


        inline bool Add(IdentifierInfo* identifierInfo, Module* module) {
            if (modules.count(identifierInfo))
                return false;
            modules.insert({ identifierInfo, module });
            return true;
        }

        inline Module* Get(IdentifierInfo* identifierInfo) {
            if (!modules.count(identifierInfo))
                return nullptr;
            return modules.at(identifierInfo);
        }

        inline llvm::DenseMap<IdentifierInfo*, Module*>::const_iterator begin() const { return modules.begin(); }
        inline llvm::DenseMap<IdentifierInfo*, Module*>::const_iterator end() const { return modules.end(); }

    private:
        llvm::DenseMap<IdentifierInfo*, Module*> modules{};
    };

}