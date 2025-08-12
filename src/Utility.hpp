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

    static TypeInfo::TypeName GetScalarTypeName(TypeInfo::TypeName type) {
        switch (type) {
            case TypeInfo::TypeName::VectorFloat: return TypeInfo::TypeName::Float;
            case TypeInfo::TypeName::VectorInt: return TypeInfo::TypeName::Int;
            case TypeInfo::TypeName::VectorBool: return TypeInfo::TypeName::Bool;
            case TypeInfo::TypeName::VectorDouble: return TypeInfo::TypeName::Double;
            default: return type;
        }
    }
}