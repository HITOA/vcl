#pragma once

#include <VCL/Error.hpp>
#include <VCL/Definition.hpp>

#include "Handle.hpp"
#include "Type.hpp"

#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/DIBuilder.h>

#include <unordered_map>
#include <string>
#include <expected>


namespace VCL {
    class ModuleContext;

    class StructDefinition {
    public:
        StructDefinition() = delete;
        StructDefinition(llvm::StructType* type, llvm::DIType* diType, 
            std::unordered_map<std::string, uint32_t>& fields, std::vector<Type> fieldsTypeInfo);
        StructDefinition(const StructDefinition& value) = default;
        StructDefinition(StructDefinition&& value) noexcept = default;
        virtual ~StructDefinition() = default;

        StructDefinition& operator=(const StructDefinition& value) = default;
        StructDefinition& operator=(StructDefinition&& value) noexcept = default;


        llvm::StructType* GetType();
        llvm::DIType* GetDIType();

        uint32_t GetFieldCount();
        uint32_t GetFieldIndex(std::string_view name);
        Type GetFieldType(uint32_t index);
        bool HasField(std::string_view name);
        

        static std::expected<Handle<StructDefinition>, Error> Create(std::string_view name, 
            std::vector<std::pair<std::string, std::shared_ptr<TypeInfo>>>& elements, ModuleContext* context);
    private:
        llvm::StructType* type;
        llvm::DIType* diType;
        std::unordered_map<std::string, uint32_t> fields;
        std::vector<Type> fieldsTypeInfo;
    };

}