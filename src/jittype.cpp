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
        case VCL::ASTTypeInfo::TypeName::INT:
            type = llvm::Type::getInt32Ty(*context);
            break;
        case VCL::ASTTypeInfo::TypeName::VFLOAT:
            type = llvm::FixedVectorType::get(llvm::Type::getFloatTy(*context), GetMaxVectorWidth() / sizeof(float));
            break;
        default:
            return llvm::Type::getVoidTy(*context);
    }

    if (typeInfo.arraySize == 0 && !(typeInfo.qualifiers & ASTTypeInfo::QualifierFlag::ARRAY))
        return type;

    // This type is a buffer

    uint32_t alignment = GetMaxVectorWidth() / (type->getScalarSizeInBits() / 8);

    size_t alignedSize = typeInfo.arraySize;
    if (alignedSize % alignment != 0)
        alignedSize = typeInfo.arraySize - (typeInfo.arraySize % alignment) + alignment;
    size_t paddedAlignedSize = alignedSize + alignment * 2;
    
    llvm::ArrayType* arrayType = llvm::ArrayType::get(type, paddedAlignedSize);

    if (typeInfo.qualifiers & ASTTypeInfo::QualifierFlag::ARRAY)
        return arrayType;

    // This type is a ring buffer

    llvm::Type* currentIndexType = llvm::Type::getInt32Ty(*context);

    llvm::StructType* bufferType = llvm::StructType::create(*context, { currentIndexType, arrayType });

    return bufferType;
}

llvm::Type* VCL::JITType::GetBaseType(llvm::Value* value) {
    if (value->getType()->isVectorTy())
        return value->getType()->getScalarType();
    if (value->getType()->isPointerTy() && llvm::isa<llvm::GlobalVariable>(value))
        return llvm::cast<llvm::GlobalVariable>(value)->getValueType();

    return value->getType();
}

llvm::Value* VCL::JITType::CastRHSToLHS(llvm::Type* lhsType, llvm::Value* rhs, llvm::IRBuilder<>* builder) {
    llvm::Type* rhsType = rhs->getType();

    if (lhsType->getTypeID() != rhsType->getTypeID()) {
        if (lhsType->isIntegerTy() && rhsType->isFloatTy())
            rhs = builder->CreateFPToSI(rhs, lhsType);
        else if (lhsType->isFloatTy() && rhsType->isIntegerTy())
            rhs = builder->CreateSIToFP(rhs, lhsType);
        else if (lhsType->isIntegerTy() && rhsType->isIntegerTy()) {
            uint32_t lhsWidth = lhsType->getIntegerBitWidth();
            uint32_t rhsWidth = rhsType->getIntegerBitWidth();
            if (lhsWidth < rhsWidth)
                builder->CreateTrunc(rhs, lhsType);
            else
                builder->CreateZExt(rhs, lhsType);
        }

        rhsType = rhs->getType();

        if (lhsType->getTypeID() == rhsType->getTypeID())
            return rhs;

        llvm::Type* lhsBaseType = lhsType->getScalarType();
        llvm::Type* rhsBaseType = GetBaseType(rhs);

        if (lhsBaseType->getTypeID() == rhsBaseType->getTypeID()) {
            if (lhsType->isVectorTy()) {
                return builder->CreateVectorSplat(GetMaxVectorWidth() / (rhsType->getScalarSizeInBits() / 8), rhs);
            } else {
                throw std::runtime_error{ "Cannot implicitly cast vector type to non vector type." };
            }
        } else {
            throw std::runtime_error{ "Unable to handle this implicit cast." };
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