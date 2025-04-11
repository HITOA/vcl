#pragma once

#include <VCL/Error.hpp>
#include <VCL/Definition.hpp>

#include "Handle.hpp"
#include "Type.hpp"

#include <llvm/IR/Value.h>

#include <expected>


namespace VCL {
    class ModuleContext;

    /**
     * @brief Represent a llvm::Value attached to a specific module
     */
    class Value {
    public:
        Value();
        Value(llvm::Value* value, Type type, ModuleContext* context);
        Value(const Value& value) = default;
        Value(Value&& value) noexcept = default;
        virtual ~Value() = default;

        Value& operator=(const Value& value) = default;
        Value& operator=(Value&& value) noexcept = default;

        /**
         * @brief Load this Value if needed and return it.
         *
         * @return Dereferenced pointer or this value if not a pointer.
         */
        std::expected<Handle<Value>, Error> Load();
        
        /**
         * @brief Store the given Value into the current Value.
         */
        void Store(Handle<Value> value);
        
        /**
         * @brief Check if this Value can be assigned by the given one.
         * 
         * This is true if both are of the same type and if this value isn't const.
         */
        bool IsAssignableFrom(Handle<Value> value) const;
        
        /**
         * @brief Cast this Value to the given type and return the casted Value.
         *
         * If both current value's type and given type are numeric. it will correctly cast
         * this value to the newly given type.
         * If the given type is a vector but not the current value, 
         * this will also broadcast the current value to a new vector.
         * If the cast can't be made, an error will be returned.
         */
        std::expected<Handle<Value>, Error> Cast(Type type);

        /**
         * @brief Check if this is a valid Value.
         *
         * A Value is not valid when it's underlying LLVM value is nullptr.
         * An invalid Value mean an error occured at some point (ex: casting).
         */
        bool IsValid() const;

        /** 
         * @brief Get the underlying LLVM value. 
         * 
         * Note: The returned llvm::Value* is managed by the LLVMContext and
         * should not be manually freed.
         */
        llvm::Value* GetLLVMValue() const;
        
        /**
         * @brief Get the underlying VCL Type.
         */
        Type GetType() const;

        /**
         * @brief Get the underlying ModuleContext
         */
        ModuleContext* GetModuleContext() const;

        /**
         * @brief Create a new non const Value with the given LLVM value, VCL Type, and context.
         */
        static std::expected<Handle<Value>, Error> Create(llvm::Value* value, Type type, ModuleContext* context);

        /**
         * @brief Create a new value that is a GlobalVariable with the given VCL Type, Value Initializer, and context.
         */
        static std::expected<Handle<Value>, Error> CreateGlobalVariable(Type type, Handle<Value> initializer, ModuleContext* context, const char* name = nullptr);

        /**
         * @brief Create a new value that is a AllocaInst with the given VCL Type, Value Initializer, and context.
         */
        static std::expected<Handle<Value>, Error> CreateLocalVariable(Type type, Handle<Value> initializer, ModuleContext* context, const char* name = nullptr);
        
        /**
         * @brief Create a new compile time constant int Value with the given int and context.
         */
        static std::expected<Handle<Value>, Error> CreateConstantInt32(int value, ModuleContext* context);

        /**
         * @brief Create a new compile time constant boolean value with the given bool and context.
         */
        static std::expected<Handle<Value>, Error> CreateConstantInt1(bool value, ModuleContext* context);

        /**
         * @brief Create a new compile time constant float value with the given float and context.
         */
        static std::expected<Handle<Value>, Error> CreateConstantFloat(float value, ModuleContext* context);

        /**
         * @brief Create a new invalid value.
         */
        static std::expected<Handle<Value>, Error> CreateInvalid();

    private:
        llvm::Value* value;
        Type type;
        ModuleContext* context;
    };

}