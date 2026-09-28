#include <VCL/AST/TypeCache.hpp>

#include <VCL/AST/DeclTemplate.hpp>
#include <VCL/AST/Template.hpp>
#include <VCL/AST/ASTContext.hpp>

#include <llvm/ADT/SmallVector.h>


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

VCL::TypeCache* VCL::TypeCache::OwnerOf(Type* type) {
    if (type && type->GetOwner())
        return type->GetOwner();
    return GetTopmostParent();
}

VCL::TypeCache* VCL::TypeCache::PickOwner(llvm::ArrayRef<TypeCache*> componentOwners) {
    TypeCache* root = GetTopmostParent();
    for (TypeCache* owner : componentOwners)
        if (owner != root)
            return owner;
    return root;
}

VCL::BuiltinType* VCL::TypeCache::GetOrCreateBuiltinType(BuiltinType::Kind kind) {
    return GetOrCreateIn<BuiltinType>(GetTopmostParent(), kind);
}

VCL::ReferenceType* VCL::TypeCache::GetOrCreateReferenceType(QualType type) {
    return GetOrCreateIn<ReferenceType>(PickOwner({ OwnerOf(type.GetType()) }), type);
}

VCL::VectorType* VCL::TypeCache::GetOrCreateVectorType(QualType ofType) {
    return GetOrCreateIn<VectorType>(GetTopmostParent(), ofType);
}

VCL::LanesType* VCL::TypeCache::GetOrCreateLanesType(QualType ofType) {
    return GetOrCreateIn<LanesType>(GetTopmostParent(), ofType);
}

VCL::ArrayType* VCL::TypeCache::GetOrCreateArrayType(QualType ofType, uint64_t ofSize) {
    return GetOrCreateIn<ArrayType>(PickOwner({ OwnerOf(ofType.GetType()) }), ofType, ofSize);
}

VCL::SpanType* VCL::TypeCache::GetOrCreateSpanType(QualType ofType) {
    return GetOrCreateIn<SpanType>(PickOwner({ OwnerOf(ofType.GetType()) }), ofType);
}

VCL::RecordType* VCL::TypeCache::GetOrCreateRecordType(RecordDecl* decl) {
    // A record type is keyed by its declaration, which belongs to one AST; it is first created
    // when the declaration is (RecordDecl::Create), in that AST's cache.
    return GetOrCreate<RecordType>(decl);
}

VCL::FunctionType* VCL::TypeCache::GetOrCreateFunctionType(QualType returnType, llvm::ArrayRef<QualType> paramsType) {
    llvm::SmallVector<TypeCache*, 8> owners{ OwnerOf(returnType.GetType()) };
    for (QualType param : paramsType)
        owners.push_back(OwnerOf(param.GetType()));
    size_t size = FunctionType::totalSizeToAlloc<QualType>(paramsType.size());
    return GetOrCreateTrailingIn<FunctionType>(PickOwner(owners), size, returnType, paramsType);
}

VCL::TemplateTypeParamType* VCL::TypeCache::GetOrCreateTemplateTypeParamType(TemplateTypeParamDecl* decl) {
    return GetOrCreateIn<TemplateTypeParamType>(this, decl);
}

VCL::TemplateSpecializationType* VCL::TypeCache::GetOrCreateTemplateSpecializationType(TemplateDecl* decl, TemplateArgumentList* args) {
    // Arguments first: an argument type from a short-lived AST (a node) must not end up in the
    // template's long-lived cache (a library).
    llvm::SmallVector<TypeCache*, 8> owners{};
    bool hasExpression = false;
    for (const TemplateArgument& arg : args->GetArgs()) {
        if (arg.GetKind() == TemplateArgument::Type)
            owners.push_back(OwnerOf(arg.GetType().GetType()));
        else if (arg.GetKind() == TemplateArgument::Expression)
            hasExpression = true;
    }
    owners.push_back(&decl->GetASTContext().GetTypeCache());
    // An unevaluated expression argument points into the caller's AST: keep such types local.
    TypeCache* owner = hasExpression ? this : PickOwner(owners);

    llvm::FoldingSetNodeID id{};
    TemplateSpecializationType::Profile(id, decl, args);
    void* insertPos = nullptr;
    if (TemplateSpecializationType* type = owner->templateSpecializationTypeTypeCache.FindNodeOrInsertPos(id, insertPos))
        return type;

    // The type keeps its argument list, which was allocated in the caller's ASTContext: give the
    // owner its own copy so the type doesn't outlive it.
    if (owner != this)
        args = TemplateArgumentList::Create(owner->typeAllocator, args->GetArgs(), args->GetSourceRange());

    void* ptr = owner->typeAllocator.Allocate(sizeof(TemplateSpecializationType), alignof(TemplateSpecializationType));
    TemplateSpecializationType* type = new(ptr) TemplateSpecializationType{ decl, args };
    type->SetOwner(owner);
    owner->templateSpecializationTypeTypeCache.InsertNode(type, insertPos);
    return type;
}

VCL::DependentType* VCL::TypeCache::GetOrCreateDependentType() {
    return GetOrCreateIn<DependentType>(GetTopmostParent());
}

VCL::TypeAliasType* VCL::TypeCache::GetOrCreateTypeAliasType(Type* ofType, TypeAliasDecl* decl) {
    // Aliases compare through their canonical type, so a per-AST copy is fine.
    return GetOrCreateIn<TypeAliasType>(this, ofType, decl);
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

    void* ptr = typeAllocator.Allocate(sizeof(T), alignof(T));
    type = new(ptr) T{ std::forward<Args>(args)... };
    type->SetOwner(this);
    GetSet<T>()->InsertNode(type, insertPos);
    return type;
}

template<typename T, typename... Args>
T* VCL::TypeCache::GetOrCreateIn(TypeCache* owner, Args&&... args) {
    llvm::FoldingSetNodeID id{};
    T::Profile(id, args...);
    void* insertPos = nullptr;
    if (T* type = owner->GetSet<T>()->FindNodeOrInsertPos(id, insertPos))
        return type;

    void* ptr = owner->typeAllocator.Allocate(sizeof(T), alignof(T));
    T* type = new(ptr) T{ std::forward<Args>(args)... };
    type->SetOwner(owner);
    owner->GetSet<T>()->InsertNode(type, insertPos);
    return type;
}

template<typename T, typename... Args>
T* VCL::TypeCache::GetOrCreateTrailingIn(TypeCache* owner, size_t totalSizeToAlloc, Args&&... args) {
    llvm::FoldingSetNodeID id{};
    T::Profile(id, args...);
    void* insertPos = nullptr;
    if (T* type = owner->GetSet<T>()->FindNodeOrInsertPos(id, insertPos))
        return type;

    void* ptr = owner->typeAllocator.Allocate(totalSizeToAlloc, 8);
    T* type = new(ptr) T{ std::forward<Args>(args)... };
    type->SetOwner(owner);
    owner->GetSet<T>()->InsertNode(type, insertPos);
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