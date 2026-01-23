#pragma once

#include <VCL/AST/Type.hpp>

#include <llvm/ADT/FoldingSet.h>
#include <llvm/ADT/IntrusiveRefCntPtr.h>
#include <llvm/Support/Allocator.h>


namespace VCL {

    /**
     * Store VCL Types into cache (FoldingSet). 
     * Provide an interface to get or create types.
     */
    class TypeCache : public llvm::RefCountedBase<TypeCache> {
    public:
        TypeCache() = default;
        TypeCache(const TypeCache& other) = delete;
        TypeCache(TypeCache&& other) = delete;
        ~TypeCache() = default;

        TypeCache& operator=(const TypeCache& other) = delete;
        TypeCache& operator=(TypeCache&& other) = delete;


        /** Get a BuiltinType of the given kind. if it does not exists, create it and return it instead. */
        BuiltinType* GetOrCreateBuiltinType(BuiltinType::Kind kind);
        /** Get a ReferenceType of the given type. if it does not exists, create it and return it instead. */
        ReferenceType* GetOrCreateReferenceType(QualType type);
        /** Get a VectorType of the given type. if it does not exists, create it and return it instead. */
        VectorType* GetOrCreateVectorType(QualType ofType);
        /** Get a ArrayType of the given type and size. if it does not exists, create it and return it instead. */
        ArrayType* GetOrCreateArrayType(QualType ofType, uint64_t ofSize);
        /** Get a SpanType of the given type. if it does not exists, create it and return it instead. */
        SpanType* GetOrCreateSpanType(QualType ofType);
        /** Get a RecordType of the given decl. if it does not exists, create it and return it instead. */
        RecordType* GetOrCreateRecordType(RecordDecl* decl);
        /** Get a FunctionType of the given args. if it does not exists, create it and return it instead. */
        FunctionType* GetOrCreateFunctionType(QualType returnType, llvm::ArrayRef<QualType> paramsType);
        /** Get a TemplateTypeParamType of the given decl. if it does not exists, create it and return it instead. */
        TemplateTypeParamType* GetOrCreateTemplateTypeParamType(TemplateTypeParamDecl* decl);
        /** Get a TemplateSpecializationType of the given decl and argument list. if it does not exists, create it and return it instead. */
        TemplateSpecializationType* GetOrCreateTemplateSpecializationType(TemplateDecl* decl, TemplateArgumentList* args);
        /** Get a DependentType */
        DependentType* GetOrCreateDependentType();

    private:
        llvm::BumpPtrAllocator typeAllocator{};
        llvm::FoldingSet<BuiltinType> builtinTypeCache{};
        llvm::FoldingSet<ReferenceType> referenceTypeCache{};
        llvm::FoldingSet<VectorType> vectorTypeCache{};
        llvm::FoldingSet<ArrayType> arrayTypeCache{};
        llvm::FoldingSet<SpanType> spanTypeCache{};
        llvm::FoldingSet<RecordType> recordTypeCache{};
        llvm::FoldingSet<FunctionType> functionTypeCache{};
        llvm::FoldingSet<TemplateTypeParamType> templateTypeParamTypeCache{};
        llvm::FoldingSet<TemplateSpecializationType> templateSpecializationTypeTypeCache{};
        llvm::FoldingSet<DependentType> dependentTypeCache{};
    };

}