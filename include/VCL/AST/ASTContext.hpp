#pragma once

#include <VCL/AST/TypeCache.hpp>

#include <llvm/Support/Allocator.h>
#include <llvm/ADT/IntrusiveRefCntPtr.h>


namespace VCL {

    class TranslationUnitDecl;

    /**
     * This is the AST Context, it hold all the nodes, allocate them, and free them all at once on destruction.
     * It also contain the TypeCache needed for the nodes wich will be freed at destruction too.
     */
    class ASTContext : public llvm::RefCountedBase<ASTContext> {
    public:
        ASTContext();
        ASTContext(const ASTContext& other) = delete;
        ASTContext(ASTContext&& other) = delete;
        ~ASTContext() = default;

        ASTContext& operator=(const ASTContext& other) = delete;
        ASTContext& operator=(ASTContext&& other) = delete;

        inline TypeCache& GetTypeCache() { return typeCache; }

        template<typename T, typename... Args>
        inline T* AllocateNode(Args&&... args) {
            void* ptr = nodeAllocator.Allocate(sizeof(T), 4);
            return new (ptr) T{ std::forward<Args>(args)... };
        }

        inline void* Allocate(size_t size) {
            return nodeAllocator.Allocate(size, 4);
        }

        inline TranslationUnitDecl* GetTranslationUnitDecl() { return root; }
        
    private:
        llvm::BumpPtrAllocator nodeAllocator{};
        TypeCache typeCache{};

        // Root translation unit decl of this AST
        TranslationUnitDecl* root = nullptr;
    };

}