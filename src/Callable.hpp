#pragma once

#include <VCL/Error.hpp>

#include "Type.hpp"
#include "Value.hpp"

#include <expected>
#include <vector>

#include <llvm/IR/Function.h>
#include <llvm/IR/DIBuilder.h>


namespace VCL {

    enum class CallableType {
        Function,
        Intrinsic
    };

    /**
     * @brief Base class for a callable Value.
     */
    class Callable : public Value {
    public:
        Callable(llvm::Value* value, Type type, ModuleContext* context) : Value{ value, type, context } {}
        virtual ~Callable() = default;

        /**
         * @brief Emit LLVM IR for this callable witht the given arguments.
         * 
         * @return The result of the call or an error.
         */
        virtual std::expected<Handle<Value>, Error> Call(std::vector<Handle<Value>>& argsv) = 0;

        /**
         * @brief Check if the given type can be taken as argument given the index.
         * 
         * This might be better to use instead of manually checking with GetArgType
         * because some callable might support more than one arguments type.
         */
        virtual bool CheckArgType(uint32_t index, Handle<Value> value) = 0;

        /**
         * @brief Check if the given number of argument are valid for this callable.
         */
        virtual bool CheckArgCount(uint32_t count) = 0;

        /**
         * @brief Get the callable type. can be used for casting into full type.
         */
        virtual CallableType GetCallableType() = 0;
    };

    class Function : public Callable {
    public:
        struct ArgInfo {
            Type type;
            std::string name;
        };

    public:
        Function() = delete;
        Function(llvm::Function* value, Type type, Type returnType, llvm::DISubroutineType* diType, std::vector<ArgInfo>& argsInfo, ModuleContext* context);
        Function(const Function& value) = default;
        Function(Function&& value) noexcept = default;
        virtual ~Function() = default;

        Function& operator=(const Function& value) = default;
        Function& operator=(Function&& value) noexcept = default;
        
        std::expected<Handle<Value>, Error> Call(std::vector<Handle<Value>>& argsv) override;

        bool CheckArgType(uint32_t index, Handle<Value> value) override;

        bool CheckArgCount(uint32_t count) override;

        CallableType GetCallableType() override;

        /**
         * @brief Get function's return type.
         */
        Type GetReturnType();

        /**
         * @brief Get function's arguments count.
         */
        uint32_t GetArgCount();

        /**
         * @brief Get function's argument type given the index.
         */
        Type GetArgType(uint32_t index);

        /**
         * @brief Get function's args info
         */
        const std::vector<ArgInfo>& GetArgsInfo() const;

        /**
         * @brief Get underlying LLVM Function.
         */
        llvm::Function* GetLLVMFunction() const;

        /**
         * @brief Get underlying LLVM Debug Information Type.
         */
        llvm::DISubroutineType* GetDIType() const;

        /**
         * @brief Check if this function have storage
         */
        bool HasStorage() const;

        /**
         * @brief Verify if the function is valid and throw an error if not
         */
        void Verify() const;

        /**
         * @brief Create a new empty function
         */
        static std::expected<Handle<Function>, Error> Create(Type returnType, std::vector<ArgInfo>& argsInfo, std::string_view name, ModuleContext* context);

    private:
        Type returnType;
        llvm::DISubroutineType* diType;
        std::vector<ArgInfo> argsType;
    };
}