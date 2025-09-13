#pragma once

#include <VCL/AST/Type.hpp>

#include <llvm/ADT/FoldingSet.h>
#include <llvm/Support/Allocator.h>


namespace VCL {

    /**
     * Store VCL Types into cache (FoldingSet). 
     * Provide an interface to get or create types.
     */
    class TypeCache {
    public:
        TypeCache() = default;
        TypeCache(const TypeCache& other) = delete;
        TypeCache(TypeCache&& other) = delete;
        ~TypeCache() = default;

        TypeCache& operator=(const TypeCache& other) = delete;
        TypeCache& operator=(TypeCache&& other) = delete;


        /** Get a BuiltinType of the given kind. if it does not exists, create it and return it instead. */
        BuiltinType* GetOrCreateBuiltinType(BuiltinType::Kind kind);
        /** Get a VectorType of the given type. if it does not exists, create it and return it instead. */
        VectorType* GetOrCreateVectorType(QualType ofType);
        /** Get a ArrayType of the given type and size. if it does not exists, create it and return it instead. */
        ArrayType* GetOrCreateArrayType(QualType ofType, uint64_t ofSize);
        /** Get a TemplateTypeParamType of the given decl. if it does not exists, create it and return it instead. */
        TemplateTypeParamType* GetOrCreateTemplateTypeParamType(TemplateTypeParamDecl* decl);
        /** Get a TemplateSpecializationType of the given decl and argument list. if it does not exists, create it and return it instead. */
        TemplateSpecializationType* GetOrCreateTemplateSpecializationType(TemplateDecl* decl, TemplateArgumentList* args);

    private:
        llvm::BumpPtrAllocator typeAllocator{};
        llvm::FoldingSet<BuiltinType> builtinTypeCache{};
        llvm::FoldingSet<VectorType> vectorTypeCache{};
        llvm::FoldingSet<ArrayType> arrayTypeCache{};
        llvm::FoldingSet<TemplateTypeParamType> templateTypeParamTypeCache{};
        llvm::FoldingSet<TemplateSpecializationType> templateSpecializationTypeTypeCache{};
    };

}