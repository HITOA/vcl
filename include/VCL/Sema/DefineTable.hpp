#pragma once

#include <VCL/Core/Identifier.hpp>
#include <VCL/AST/ConstantValue.hpp>

#include <llvm/ADT/DenseMap.h>


namespace VCL {

    class DefineTable : public llvm::RefCountedBase<DefineTable> {
    public:
        DefineTable() = default;
        DefineTable(const DefineTable& other) = delete;
        DefineTable(DefineTable&& other) = delete;
        ~DefineTable() = default;

        DefineTable& operator=(const DefineTable& other) = delete;
        DefineTable& operator=(DefineTable&& other) = delete;


        inline bool Add(IdentifierInfo* identifierInfo, ConstantValue* value) {
            if (values.count(identifierInfo))
                return false;
            values.insert({ identifierInfo, value });
            return true;
        }

        inline ConstantValue* Get(IdentifierInfo* identifierInfo) {
            if (!values.count(identifierInfo))
                return nullptr;
            return values.at(identifierInfo);
        }

        inline llvm::DenseMap<IdentifierInfo*, ConstantValue*>::const_iterator begin() const { return values.begin(); }
        inline llvm::DenseMap<IdentifierInfo*, ConstantValue*>::const_iterator end() const { return values.end(); }

    private:
        llvm::DenseMap<IdentifierInfo*, ConstantValue*> values{};
    };

}