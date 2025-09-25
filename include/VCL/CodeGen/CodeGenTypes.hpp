#pragma once

#include <VCL/AST/Type.hpp>

#include <llvm/IR/Type.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/ADT/DenseMap.h>


namespace VCL {
    class CodeGenModule;

    class CodeGenTypes {
    public:
        CodeGenTypes(CodeGenModule& cgm) : cgm{ cgm} {}
        CodeGenTypes(const CodeGenTypes& other) = delete;
        CodeGenTypes(CodeGenTypes&& other) = delete;
        ~CodeGenTypes() = default;

        CodeGenTypes& operator=(const CodeGenTypes& other) = delete;
        CodeGenTypes& operator=(CodeGenTypes&& other) = delete;

        
        llvm::Type* ConvertType(QualType type);

    private:
        llvm::Type* ConvertBuiltinType(QualType type);
        llvm::VectorType* ConvertVectorType(QualType type);
        llvm::ArrayType* ConvertArrayType(QualType type);
        llvm::StructType* ConvertRecordDeclType(QualType type);

    private:
        llvm::DenseMap<Type*, llvm::Type*> types;
        CodeGenModule& cgm;
    };
    
}