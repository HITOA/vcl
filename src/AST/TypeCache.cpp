#include <VCL/AST/TypeCache.hpp>



void VCL::TypeCache::InsertTypeCacheChild(TypeCache* child) {
    auto it = std::find(childs.begin(), childs.end(), child);
    assert(it == childs.end());
    child->parent = this;
    childs.push_back(child);
}

void VCL::TypeCache::RemoveTypecacheChild(TypeCache* child) {
    auto it = std::find(childs.begin(), childs.end(), child);
    assert(it != childs.end());
    child->parent = nullptr;
    childs.erase(it);
}

VCL::BuiltinType* VCL::TypeCache::GetOrCreateBuiltinType(BuiltinType::Kind kind) {
    TypeCache* topmostCache = GetTopmostParent();
    return topmostCache->GetOrCreate<BuiltinType>(kind);
}

VCL::ReferenceType* VCL::TypeCache::GetOrCreateReferenceType(QualType type) {
    return GetOrCreate<ReferenceType>(type);
}

VCL::VectorType* VCL::TypeCache::GetOrCreateVectorType(QualType ofType) {
    TypeCache* topmostCache = GetTopmostParent();
    return topmostCache->GetOrCreate<VectorType>(ofType);
}

VCL::LanesType* VCL::TypeCache::GetOrCreateLanesType(QualType ofType) {
    TypeCache* topmostCache = GetTopmostParent();
    return topmostCache->GetOrCreate<LanesType>(ofType);
}

VCL::ArrayType* VCL::TypeCache::GetOrCreateArrayType(QualType ofType, uint64_t ofSize) {
    return GetOrCreate<ArrayType>(ofType, ofSize);
}

VCL::SpanType* VCL::TypeCache::GetOrCreateSpanType(QualType ofType) {
    return GetOrCreate<SpanType>(ofType);
}

VCL::RecordType* VCL::TypeCache::GetOrCreateRecordType(RecordDecl* decl) {
    return GetOrCreate<RecordType>(decl);
}

VCL::FunctionType* VCL::TypeCache::GetOrCreateFunctionType(QualType returnType, llvm::ArrayRef<QualType> paramsType) {
    size_t size = FunctionType::totalSizeToAlloc<QualType>(paramsType.size());
    return GetOrCreateTrailing<FunctionType>(size, returnType, paramsType);
}

VCL::TemplateTypeParamType* VCL::TypeCache::GetOrCreateTemplateTypeParamType(TemplateTypeParamDecl* decl) {
    return GetOrCreate<TemplateTypeParamType>(decl);
}

VCL::TemplateSpecializationType* VCL::TypeCache::GetOrCreateTemplateSpecializationType(TemplateDecl* decl, TemplateArgumentList* args) {
    return GetOrCreate<TemplateSpecializationType>(decl, args);
}

VCL::DependentType* VCL::TypeCache::GetOrCreateDependentType() {
    return GetOrCreate<DependentType>();
}

VCL::TypeAliasType* VCL::TypeCache::GetOrCreateTypeAliasType(Type* ofType, TypeAliasDecl* decl) {
    return GetOrCreate<TypeAliasType>(ofType, decl);
}

template<typename T, typename... Args>
T* VCL::TypeCache::GetOrCreate(Args&&... args) {
    llvm::FoldingSetNodeID id{};
    T::Profile(id, std::forward<Args>(args)...);
    void* insertPos = nullptr;
    T* type = GetSet<T>()->FindNodeOrInsertPos(id, insertPos);

    if (type != nullptr)
        return type;

    type = GetTopmostParent()->FindType<T, Args...>(id, std::forward<Args>(args)...);
    if (type != nullptr)
        return type;

    void* ptr = typeAllocator.Allocate(sizeof(T), 4);
    type = new(ptr) T{ std::forward<Args>(args)... };
    GetSet<T>()->InsertNode(type, insertPos);
    return type;
}

template<typename T, typename... Args>
T* VCL::TypeCache::GetOrCreateTrailing(size_t totalSizeToAlloc, Args&&... args) {
    llvm::FoldingSetNodeID id{};
    T::Profile(id, std::forward<Args>(args)...);
    void* insertPos = nullptr;
    T* type = GetSet<T>()->FindNodeOrInsertPos(id, insertPos);

    type = GetTopmostParent()->FindType<T, Args...>(id, std::forward<Args>(args)...);
    if (type != nullptr)
        return type;

    void* ptr = typeAllocator.Allocate(totalSizeToAlloc, 4);
    type = new(ptr) T{ std::forward<Args>(args)... };
    GetSet<T>()->InsertNode(type, insertPos);
    return type;
}

template<typename T, typename... Args>
T* VCL::TypeCache::FindType(llvm::FoldingSetNodeID& id, Args&&... args) {
    void* insertPos = nullptr;
    T* type = GetSet<T>()->FindNodeOrInsertPos(id, insertPos);

    if (type != nullptr)
        return type;

    for (auto child : childs) {
        type = child->FindType<T, Args...>(id, std::forward<Args>(args)...);
        if (type != nullptr)
            return type;
    }

    return nullptr;
}

template<typename T>
llvm::FoldingSet<T>* VCL::TypeCache::GetSet() {
    assert(false && "unsupported typecache type");
    return nullptr;
}

template<>
llvm::FoldingSet<VCL::BuiltinType>* VCL::TypeCache::GetSet<VCL::BuiltinType>() {
    return &builtinTypeCache;
}

template<>
llvm::FoldingSet<VCL::ReferenceType>* VCL::TypeCache::GetSet<VCL::ReferenceType>() {
    return &referenceTypeCache;
}

template<>
llvm::FoldingSet<VCL::VectorType>* VCL::TypeCache::GetSet<VCL::VectorType>() {
    return &vectorTypeCache;
}

template<>
llvm::FoldingSet<VCL::LanesType>* VCL::TypeCache::GetSet<VCL::LanesType>() {
    return &lanesTypeCache;
}

template<>
llvm::FoldingSet<VCL::ArrayType>* VCL::TypeCache::GetSet<VCL::ArrayType>() {
    return &arrayTypeCache;
}

template<>
llvm::FoldingSet<VCL::SpanType>* VCL::TypeCache::GetSet<VCL::SpanType>() {
    return &spanTypeCache;
}

template<>
llvm::FoldingSet<VCL::RecordType>* VCL::TypeCache::GetSet<VCL::RecordType>() {
    return &recordTypeCache;
}

template<>
llvm::FoldingSet<VCL::FunctionType>* VCL::TypeCache::GetSet<VCL::FunctionType>() {
    return &functionTypeCache;
}

template<>
llvm::FoldingSet<VCL::TemplateTypeParamType>* VCL::TypeCache::GetSet<VCL::TemplateTypeParamType>() {
    return &templateTypeParamTypeCache;
}

template<>
llvm::FoldingSet<VCL::TemplateSpecializationType>* VCL::TypeCache::GetSet<VCL::TemplateSpecializationType>() {
    return &templateSpecializationTypeTypeCache;
}

template<>
llvm::FoldingSet<VCL::DependentType>* VCL::TypeCache::GetSet<VCL::DependentType>() {
    return &dependentTypeCache;
}

template<>
llvm::FoldingSet<VCL::TypeAliasType>* VCL::TypeCache::GetSet<VCL::TypeAliasType>() {
    return &typeAliasTypeCache;
}