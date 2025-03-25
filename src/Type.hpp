#pragma once

#include <VCL/Definition.hpp>

#include "ModuleContext.hpp"

#include <llvm/IR/Type.h>


namespace VCL {

    /**
     * @brief represents a VCL Type.
     */
    class Type {
    public:
        Type() = delete;
        Type(TypeInfo typeInfo, llvm::Type* type, ModuleContext* context);
        Type(const Type& type) = default;
        Type(Type&& type) noexcept = default;
        ~Type() = default;

        Type& operator=(const Type& value) = default;
        Type& operator=(Type&& value) noexcept = default;

        /**
         * @brief Get the underlying TypeInfo.
         */
        TypeInfo GetTypeInfo() const;

        /**
         * @brief Get the underlying LLVM Type.
         */
        llvm::Type* GetLLVMType() const;

        /**
         * @brief Compare types for equality
         */
        bool operator==(Type& rhs) const;

        /**
         * @brief Compare types for inequality
         */
        bool operator!=(Type& rhs) const;

        /**
         * @brief Create a full Type from a TypeInfo structure and a context.
         */
        static std::expected<Type, Error> Create(TypeInfo typeInfo, ModuleContext* context);

        /**
         * @brief Create a full Type from a LLVM Type and context.
         */
        static std::expected<Type, Error> CreateFromLLVMType(llvm::Type* type, ModuleContext* context);

    private:
        TypeInfo typeInfo;
        llvm::Type* type;
        ModuleContext* context;
    };

}