#pragma once

#include <VCL/Definition.hpp>
#include <VCL/Error.hpp>

#include <llvm/IR/Type.h>
#include <llvm/IR/DIBuilder.h>

#include <expected>


namespace VCL {
    class ModuleContext;
    /**
     * @brief represents a VCL Type.
     */
    class Type {
    public:
        Type();
        Type(std::shared_ptr<TypeInfo> typeInfo, llvm::Type* type, llvm::DIType* diType, ModuleContext* context, bool isPointer);
        Type(const Type& type) = default;
        Type(Type&& type) noexcept = default;
        ~Type() = default;

        Type& operator=(const Type& value) = default;
        Type& operator=(Type&& value) noexcept = default;

        bool IsPointerType() const;

        void SetPointer();

        /**
         * @brief Get the underlying TypeInfo.
         */
        std::shared_ptr<TypeInfo> GetTypeInfo() const;

        /**
         * @brief Get the underlying LLVM Type.
         */
        llvm::Type* GetLLVMType() const;

        /**
         * @brief Get the underlying LLVM Debug Information Type.
         */
        llvm::DIType* GetDIType() const;

        /**
         * @brief Compare types for equality
         */
        bool operator==(Type rhs) const;

        /**
         * @brief Compare types for inequality
         */
        bool operator!=(Type rhs) const;

        /**
         * @brief Create a full Type from a TypeInfo structure and a context.
         */
        static std::expected<Type, Error> Create(std::shared_ptr<TypeInfo> typeInfo, ModuleContext* context, bool isPointer = false);

        /**
         * @brief Create a pointer type of a full type
         */
        static std::expected<Type, Error> CreatePointerType(Type baseType);

    private:
        std::shared_ptr<TypeInfo> typeInfo;
        llvm::Type* type;
        llvm::DIType* diType;
        ModuleContext* context;
        bool isPointer;
    };

}