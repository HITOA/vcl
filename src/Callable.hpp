#pragma once

#include <VCL/Error.hpp>

#include "Type.hpp"
#include "Value.hpp"

#include <expected>
#include <vector>

#include <llvm/IR/Function.h>


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
         * @brief Get the callable return type.
         */
        virtual Type GetReturnType() = 0;

        /**
         * @brief Get the callable arguments count.
         */
        virtual uint32_t GetArgCount() = 0;

        /**
         * @brief Get the callable argument type given the index.
         */
        virtual Type GetArgType(uint32_t index) = 0;

        /**
         * @brief Check if the given type can be taken as argument given the index.
         * 
         * This might be better to use instead of manually checking with GetArgType
         * because some callable might support more than one arguments type.
         */
        virtual bool CheckArgType(uint32_t index, Type type) = 0;

        /**
         * @brief Get the callable type. can be used for casting into full type.
         */
        virtual CallableType GetCallableType() = 0;
    };

    class Function : public Callable {
    public:
        struct ArgInfo {
            Type type;
            std::string_view name;
        };

    public:
        Function() = delete;
        Function(llvm::Function* value, Type type, Type returnType, std::vector<ArgInfo>& argsInfo, ModuleContext* context);
        Function(const Function& value) = default;
        Function(Function&& value) noexcept = default;
        virtual ~Function() = default;

        Function& operator=(const Function& value) = default;
        Function& operator=(Function&& value) noexcept = default;
        
        std::expected<Handle<Value>, Error> Call(std::vector<Handle<Value>>& argsv) override;

        Type GetReturnType() override;

        uint32_t GetArgCount() override;

        Type GetArgType(uint32_t index) override;

        bool CheckArgType(uint32_t index, Type type) override;

        CallableType GetCallableType() override;

        /**
         * @brief Get function's args info
         */
        const std::vector<ArgInfo>& GetArgsInfo() const;

        /**
         * @brief Get underlying LLVM Function.
         */
        llvm::Function* GetLLVMFunction() const;

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
        std::vector<ArgInfo> argsType;
    };
}