#include <VCL/AST/TypeCache.hpp>


template<typename T, typename... Args>
static inline T* GetOrCreateInFoldingSet(llvm::BumpPtrAllocator& allocator, llvm::FoldingSet<T>& set, Args&&... args) {
    llvm::FoldingSetNodeID id{};
    T::Profile(id, std::forward<Args>(args)...);
    void* insertPos = nullptr;
    T* type = set.FindNodeOrInsertPos(id, insertPos);
    if (type == nullptr && insertPos != nullptr) {
        void* ptr = allocator.Allocate(sizeof(T), 4);
        type = new(ptr) T{ std::forward<Args>(args)... };
        set.InsertNode(type, insertPos);
    }
    return type;
}

VCL::BuiltinType* VCL::TypeCache::GetOrCreateBuiltinType(BuiltinType::Kind kind) {
    return GetOrCreateInFoldingSet(typeAllocator, builtinTypeCache, kind);
}

VCL::ReferenceType* VCL::TypeCache::GetOrCreateReferenceType(QualType type) {
    return GetOrCreateInFoldingSet(typeAllocator, referenceTypeCache, type);
}

VCL::VectorType* VCL::TypeCache::GetOrCreateVectorType(QualType ofType) {
    return GetOrCreateInFoldingSet(typeAllocator, vectorTypeCache, ofType);
}

VCL::LanesType* VCL::TypeCache::GetOrCreateLanesType(QualType ofType) {
    return GetOrCreateInFoldingSet(typeAllocator, lanesTypeCache, ofType);
}

VCL::ArrayType* VCL::TypeCache::GetOrCreateArrayType(QualType ofType, uint64_t ofSize) {
    return GetOrCreateInFoldingSet(typeAllocator, arrayTypeCache, ofType, ofSize);
}

VCL::SpanType* VCL::TypeCache::GetOrCreateSpanType(QualType ofType) {
    return GetOrCreateInFoldingSet(typeAllocator, spanTypeCache, ofType);
}

VCL::RecordType* VCL::TypeCache::GetOrCreateRecordType(RecordDecl* decl) {
    return GetOrCreateInFoldingSet(typeAllocator, recordTypeCache, decl);
}

VCL::FunctionType* VCL::TypeCache::GetOrCreateFunctionType(QualType returnType, llvm::ArrayRef<QualType> paramsType) {
    llvm::FoldingSetNodeID id{};
    FunctionType::Profile(id, returnType, paramsType);
    void* insertPos = nullptr;
    FunctionType* type = functionTypeCache.FindNodeOrInsertPos(id, insertPos);
    if (type == nullptr && insertPos != nullptr) {
        size_t size = FunctionType::totalSizeToAlloc<QualType>(paramsType.size());
        void* ptr = typeAllocator.Allocate(sizeof(FunctionType) + size, 4);
        type = new(ptr) FunctionType{ returnType, paramsType };
        functionTypeCache.InsertNode(type, insertPos);
    }
    return type;
}

VCL::TemplateTypeParamType* VCL::TypeCache::GetOrCreateTemplateTypeParamType(TemplateTypeParamDecl* decl) {
    return GetOrCreateInFoldingSet(typeAllocator, templateTypeParamTypeCache, decl);
}

VCL::TemplateSpecializationType* VCL::TypeCache::GetOrCreateTemplateSpecializationType(TemplateDecl* decl, TemplateArgumentList* args) {
    return GetOrCreateInFoldingSet(typeAllocator, templateSpecializationTypeTypeCache, decl, args);
}

VCL::DependentType* VCL::TypeCache::GetOrCreateDependentType() {
    return GetOrCreateInFoldingSet(typeAllocator, dependentTypeCache);
}