#pragma once

#include "llvmheader.hpp"
#include <string_view>


namespace VCL {

    class JITBuiltins {
    public:
        static bool IsBuiltinFunction(std::string_view name);
        static llvm::Value* CallBuiltinFunction(std::string_view name, std::vector<llvm::Value*>& args, llvm::IRBuilder<>* builder);
    };

}