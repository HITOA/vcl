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

VCL::VectorType* VCL::TypeCache::GetOrCreateVectorType(Type* ofType) {
    return GetOrCreateInFoldingSet(typeAllocator, vectorTypeCache, ofType);
}

VCL::ArrayType* VCL::TypeCache::GetOrCreateArrayType(Type* ofType, uint64_t ofSize) {
    return GetOrCreateInFoldingSet(typeAllocator, arrayTypeCache, ofType, ofSize);
}