#include "jitbuiltins.hpp"

#include "jittype.hpp"

#include <unordered_map>
#include <stdexcept>
#include <format>
#include <functional>


static llvm::Value* BuiltinUnaryIntrinsic(llvm::Intrinsic::ID it, std::vector<llvm::Value*>& argsv, llvm::IRBuilder<>* builder) {
    if (argsv.size() != 1)
        throw std::runtime_error{ "Builtin unary function takes only 1 argument." };
    return builder->CreateUnaryIntrinsic(it, argsv[0]);
}

static llvm::Value* BuiltinBinaryIntrinsic(llvm::Intrinsic::ID it, std::vector<llvm::Value*>& argsv, llvm::IRBuilder<>* builder) {
    if (argsv.size() != 2)
        throw std::runtime_error{ "Builtin binary function takes 2 arguments." };
    llvm::Value* argv1 = argsv[0];
    llvm::Value* argv2 = VCL::JITType::CastRHSToLHS(argv1->getType(), argsv[1], builder);
    return builder->CreateBinaryIntrinsic(it, argv1, argv2);
}

static llvm::Value* BuiltinTernaryIntrinsic(llvm::Intrinsic::ID it, std::vector<llvm::Value*>& argsv, llvm::IRBuilder<>* builder) {
    if (argsv.size() != 3)
        throw std::runtime_error{ "Builtin ternary function takes 3 arguments." };
    llvm::Value* argv1 = argsv[0];
    llvm::Value* argv2 = VCL::JITType::CastRHSToLHS(argv1->getType(), argsv[1], builder);
    llvm::Value* argv3 = VCL::JITType::CastRHSToLHS(argv1->getType(), argsv[2], builder);
    return builder->CreateIntrinsic(it, { argsv[0]->getType() }, { argv1, argv2, argv3 });
}

static llvm::Value* BuiltinFMod(std::vector<llvm::Value*>& argsv, llvm::IRBuilder<>* builder) {
    if (argsv.size() != 2)
        throw std::runtime_error{ "Builtin binary function takes 2 arguments." };
    llvm::Value* argv1 = argsv[0];
    llvm::Value* argv2 = VCL::JITType::CastRHSToLHS(argv1->getType(), argsv[1], builder);
    return builder->CreateFRem(argv1, argv2);
}

static llvm::Value* BuiltinSelect(std::vector<llvm::Value*>& argsv, llvm::IRBuilder<>* builder) {
    if (argsv.size() != 3)
        throw std::runtime_error{ "Builtin ternary function takes 3 arguments." };
    llvm::Value* argv1 = argsv[0];
    llvm::Value* argv2 = VCL::JITType::BroadcastIfNeeded(argsv[1], builder);
    llvm::Value* argv3 = VCL::JITType::BroadcastIfNeeded(argsv[2], builder);
    return builder->CreateSelect(argv1, argv2, argv3);
}

using namespace std::placeholders;

static std::unordered_map<std::string_view, std::function<llvm::Value*(std::vector<llvm::Value*>&,llvm::IRBuilder<>*)>> builtins{
    { "sqrt",       std::bind(BuiltinUnaryIntrinsic, llvm::Intrinsic::sqrt, _1, _2) },
    { "sin",        std::bind(BuiltinUnaryIntrinsic, llvm::Intrinsic::sin, _1, _2) },
    { "cos",        std::bind(BuiltinUnaryIntrinsic, llvm::Intrinsic::cos, _1, _2) },
    { "tan",        std::bind(BuiltinUnaryIntrinsic, llvm::Intrinsic::tan, _1, _2) },
    { "asin",       std::bind(BuiltinUnaryIntrinsic, llvm::Intrinsic::asin, _1, _2) },
    { "acos",       std::bind(BuiltinUnaryIntrinsic, llvm::Intrinsic::acos, _1, _2) },
    { "atan",       std::bind(BuiltinUnaryIntrinsic, llvm::Intrinsic::atan, _1, _2) },
    { "sinh",       std::bind(BuiltinUnaryIntrinsic, llvm::Intrinsic::sinh, _1, _2) },
    { "cosh",       std::bind(BuiltinUnaryIntrinsic, llvm::Intrinsic::cosh, _1, _2) },
    { "tanh",       std::bind(BuiltinUnaryIntrinsic, llvm::Intrinsic::tanh, _1, _2) },
    { "log",        std::bind(BuiltinUnaryIntrinsic, llvm::Intrinsic::log, _1, _2) },
    { "log10",      std::bind(BuiltinUnaryIntrinsic, llvm::Intrinsic::log10, _1, _2) },
    { "log2",       std::bind(BuiltinUnaryIntrinsic, llvm::Intrinsic::log2, _1, _2) },
    { "exp",        std::bind(BuiltinUnaryIntrinsic, llvm::Intrinsic::exp, _1, _2) },
    { "exp10",      std::bind(BuiltinUnaryIntrinsic, llvm::Intrinsic::exp10, _1, _2) },
    { "exp2",       std::bind(BuiltinUnaryIntrinsic, llvm::Intrinsic::exp2, _1, _2) },
    { "fabs",       std::bind(BuiltinUnaryIntrinsic, llvm::Intrinsic::fabs, _1, _2) },
    { "ceil",       std::bind(BuiltinUnaryIntrinsic, llvm::Intrinsic::ceil, _1, _2) },
    { "floor",      std::bind(BuiltinUnaryIntrinsic, llvm::Intrinsic::floor, _1, _2) },
    { "round",      std::bind(BuiltinUnaryIntrinsic, llvm::Intrinsic::round, _1, _2) },

    { "pow",        std::bind(BuiltinBinaryIntrinsic, llvm::Intrinsic::pow, _1, _2) },
    { "min",        std::bind(BuiltinBinaryIntrinsic, llvm::Intrinsic::minnum, _1, _2) },
    { "max",        std::bind(BuiltinBinaryIntrinsic, llvm::Intrinsic::maxnum, _1, _2) },
    { "fmod",       BuiltinFMod },

    { "fma",        std::bind(BuiltinTernaryIntrinsic, llvm::Intrinsic::fma, _1, _2) },
    { "brev",       std::bind(BuiltinTernaryIntrinsic, llvm::Intrinsic::bitreverse, _1, _2) },
    { "select",     BuiltinSelect }
};


bool VCL::JITBuiltins::IsBuiltinFunction(std::string_view name) {
    return builtins.count(name);
}

llvm::Value* VCL::JITBuiltins::CallBuiltinFunction(std::string_view name, std::vector<llvm::Value*>& args, llvm::IRBuilder<>* builder) {
    if (!builtins.count(name))
        throw std::runtime_error{ std::format("Tried to call the builtin function \"{}\" wich does not exist.", name) };

    return builtins[name](args, builder);
}