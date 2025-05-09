#pragma once

#include <VCL/Error.hpp>
#include <VCL/Definition.hpp>

#include "Handle.hpp"

#include <llvm/IR/DerivedTypes.h>

#include <unordered_map>
#include <string>
#include <expected>


namespace VCL {
    class ModuleContext;

    class StructDefinition {
    public:
        StructDefinition() = delete;
        StructDefinition(llvm::StructType* type, std::unordered_map<std::string, uint32_t>& fields);
        StructDefinition(const StructDefinition& value) = default;
        StructDefinition(StructDefinition&& value) noexcept = default;
        virtual ~StructDefinition() = default;

        StructDefinition& operator=(const StructDefinition& value) = default;
        StructDefinition& operator=(StructDefinition&& value) noexcept = default;


        llvm::StructType* GetType();

        uint32_t GetFieldCount();
        uint32_t GetFieldIndex(std::string_view name);
        bool HasField(std::string_view name);
        

        static std::expected<Handle<StructDefinition>, Error> Create(std::string_view name, 
            std::vector<std::pair<std::string, std::shared_ptr<TypeInfo>>>& elements, ModuleContext* context);
    private:
        llvm::StructType* type;
        std::unordered_map<std::string, uint32_t> fields;
    };

}