#pragma once

#include <type_traits>
#include <vector>

#include <VCL/AST/TypeCache.hpp>

#include <llvm/Support/Allocator.h>
#include <llvm/ADT/IntrusiveRefCntPtr.h>
#include <llvm/ADT/DenseMap.h>
#include <llvm/ADT/StringMap.h>

#include <string>


namespace VCL {

    class TranslationUnitDecl;
    class NamedDecl;

    /**
     * This is the AST Context, it hold all the nodes, allocate them, and free them all at once on destruction.
     * It also contain the TypeCache needed for the nodes wich will be freed at destruction too.
     */
    class ASTContext : public llvm::RefCountedBase<ASTContext> {
    public:
        ASTContext() = delete;
        ASTContext(TypeCache& typecache);
        ASTContext(const ASTContext& other) = delete;
        ASTContext(ASTContext&& other) = delete;
        ~ASTContext();

        ASTContext& operator=(const ASTContext& other) = delete;
        ASTContext& operator=(ASTContext&& other) = delete;

        inline TypeCache& GetTypeCache() { return typeCache; }

        template<typename T, typename... Args>
        inline T* AllocateNode(Args&&... args) {
            void* ptr = nodeAllocator.Allocate(sizeof(T), alignof(T));
            T* node = new (ptr) T{ std::forward<Args>(args)... };
            // The allocator frees memory without running destructors: remember the nodes that own
            // memory (strings, vectors) and destroy them with the context.
            if constexpr (!std::is_trivially_destructible_v<T>)
                destructors.push_back({ node, [](void* p) { ((T*)p)->~T(); } });
            return node;
        }

        inline void* Allocate(size_t size) {
            return nodeAllocator.Allocate(size, 8);
        }

        inline TranslationUnitDecl* GetTranslationUnitDecl() { return root; }

        /**
         * Prefix of every symbol name mangled from this AST. It must be unique among the ASTs
         * emitted into the same llvm::Module: e.g. the module's path, or, for a client emitting one
         * source several times, a name per copy.
         */
        inline const std::string& GetManglingPrefix() const { return manglingPrefix; }
        inline void SetManglingPrefix(std::string prefix) { manglingPrefix = std::move(prefix); }

        /**
         * `<prefix>.<name>`, then `<prefix>.<name>.1`, `.2`... for further declarations with the
         * same name (template specializations). Stable for a given declaration once assigned.
         */
        const std::string& GetMangledName(NamedDecl* decl);
        
    private:
        llvm::BumpPtrAllocator nodeAllocator;
        // Nodes to destroy with the context (see AllocateNode), in allocation order. Declared
        // before `root`, which the constructor allocates.
        std::vector<std::pair<void*, void(*)(void*)>> destructors{};
        TypeCache typeCache;

        // Root translation unit decl of this AST
        TranslationUnitDecl* root;

        std::string manglingPrefix{};
        llvm::DenseMap<NamedDecl*, std::string> mangledNames{};
        llvm::StringMap<uint32_t> mangledNameUses{};
    };

}