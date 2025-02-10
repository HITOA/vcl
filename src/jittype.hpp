#pragma once

#include <vcl/ast.hpp>
#include "llvmheader.hpp"

namespace VCL {

    class JITType {
    public:
        static llvm::Type* GetType(VCL::ASTTypeInfo typeInfo, llvm::LLVMContext* context);
        static llvm::Type* GetBaseType(llvm::Type* type);
        static llvm::Value* CastRHSToLHS(llvm::Type* lhsType, llvm::Value* rhs, llvm::IRBuilder<>* builder);
        static llvm::Value* BroadcastIfNeeded(llvm::Value* value, llvm::IRBuilder<>* builder);
        static size_t GetMaxVectorWidth();
    };

}