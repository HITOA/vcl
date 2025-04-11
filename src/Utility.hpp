#pragma once

#include <VCL/Definition.hpp>

#include "Type.hpp"
#include "Value.hpp"

#include <llvm/TargetParser/Host.h>

#include <initializer_list>
#include <vector>
#include <expected>
#include <functional>

#define UNARY_DISPATCH_FUNCTION(Type, Function) { TypeInfo::TypeName::Type, [&](llvm::Value* value) -> llvm::Value* \
    { return context->GetIRBuilder().Function(value); } }

#define DISPATCH_UNARY(...) Dispatcher<TypeInfo::TypeName, llvm::Value*, llvm::Value*>{ __VA_ARGS__ }

#define BINARY_DISPATCH_FUNCTION(Type, Function) { TypeInfo::TypeName::Type, [&](llvm::Value* lhs,  llvm::Value* rhs) -> llvm::Value* \
    { return context->GetIRBuilder().Function(lhs, rhs); } }

#define DISPATCH_BINARY(...) Dispatcher<TypeInfo::TypeName, llvm::Value*, llvm::Value*, llvm::Value*>{ __VA_ARGS__ }

namespace VCL {

    template<typename T, typename R, typename... Args>
    struct DispatcherFunction {
        T name;
        std::function<R(Args...)> func;
        DispatcherFunction(T name, std::function<R(Args...)> func) : name{ name }, func{ func } {};
        inline R operator()(Args... args) const { return func(args...); }
    };

    template<typename T, typename R, typename... Args>
    struct Dispatcher
    {   
        std::vector<DispatcherFunction<T, R, Args...>> funcs;
        Dispatcher(std::initializer_list<DispatcherFunction<T, R, Args...>> funcs) : funcs{ funcs } {};
        inline std::expected<R, Error> operator()(T name, Args... args) {
            for (auto& func : funcs)
                if (func.name == name)
                    return func(args...);
            return std::unexpected(Error{ "Invalid operand(s) type for this operation." });
        }
    };

    inline std::expected<Handle<Value>, Error> MakeValueVCLFromLLVM(llvm::Value* value, ModuleContext* context) {
        std::expected<Type, Error> type = Type::CreateFromLLVMType(value->getType(), context);
        if (!type.has_value())
            return std::unexpected(type.error());
        return Value::Create(value, *type, context);
    }

    inline uint32_t GetMaxVectorElementWidth(uint32_t elementSize) {
        static uint32_t maxVectorWidthInBytes = 0;
        if (maxVectorWidthInBytes == 0) {
            llvm::StringMap<bool> features = llvm::sys::getHostCPUFeatures();
            if (features["avx512f"]) maxVectorWidthInBytes = 64;
            if (features["avx2"]) maxVectorWidthInBytes = 32;
            if (features["sse2"]) maxVectorWidthInBytes = 16;
            else maxVectorWidthInBytes = 8;
        }
        return maxVectorWidthInBytes / elementSize;
    }
}