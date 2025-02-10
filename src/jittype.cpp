#include "jittype.hpp"
#include <iostream>


static size_t GetMaxVectorWidth() {
    static size_t mvw = 0;
    if (mvw == 0) {
        llvm::StringMap<bool> Features = llvm::sys::getHostCPUFeatures();

        if (Features["avx512f"]) mvw = 64;
        else if (Features["avx2"]) mvw = 32;
        else if (Features["sse2"]) mvw = 16;
        else mvw = 8;
    }
    return mvw;
}

llvm::Type* VCL::JITType::GetType(VCL::ASTTypeInfo typeInfo, llvm::LLVMContext* context) {
    llvm::Type* type;
    switch (typeInfo.type) {
        case VCL::ASTTypeInfo::TypeName::FLOAT:
            type = llvm::Type::getFloatTy(*context);
            break;
        case VCL::ASTTypeInfo::TypeName::VFLOAT:
            type = llvm::FixedVectorType::get(llvm::Type::getFloatTy(*context), GetMaxVectorWidth() / sizeof(float));
            break;
        default:
            return llvm::Type::getVoidTy(*context);
    }

    return type;
}

llvm::Type* VCL::JITType::GetBaseType(llvm::Type* type) {
    if (type->isVectorTy())
        return ((llvm::VectorType*)type)->getElementType();
    return type;
}

llvm::Value* VCL::JITType::CastRHSToLHS(llvm::Type* lhsType, llvm::Value* rhs, llvm::IRBuilder<>* builder) {
    llvm::Type* rhsType = rhs->getType();

    if (lhsType->getTypeID() != rhsType->getTypeID()) {
        llvm::Type* lhsBaseType = GetBaseType(lhsType);
        llvm::Type* rhsBaseType = GetBaseType(rhsType);

        if (lhsBaseType->getTypeID() == rhsBaseType->getTypeID()) {
            if (lhsType->isVectorTy()) {
                return builder->CreateVectorSplat(GetMaxVectorWidth() / (rhsType->getScalarSizeInBits() / 8), rhs);
            } else {
                throw std::runtime_error{ "Cannot implicitly cast vector type to non vector type." };
            }
        }
    }
    
    return rhs;
}

llvm::Value* VCL::JITType::BroadcastIfNeeded(llvm::Value* value, llvm::IRBuilder<>* builder) {
    llvm::Type* type = value->getType();
    if (type->isVectorTy())
        return value;
    return builder->CreateVectorSplat(GetMaxVectorWidth() / (type->getScalarSizeInBits() / 8), value);
}

size_t VCL::JITType::GetMaxVectorWidth() {
    return ::GetMaxVectorWidth();
}