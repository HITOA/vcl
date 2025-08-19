#pragma once

#include <VCL/AST/TypeCache.hpp>

#include <llvm/Support/Allocator.h>


namespace VCL {

    /**
     * This is the AST Context, it hold all the nodes, allocate them, and free them all at once on destruction.
     * It also contain the TypeCache needed for the nodes wich will be freed at destruction too.
     */
    class ASTContext {
    public:
        
    private:
        TypeCache typeCache{};

        llvm::BumpPtrAllocator nodeAllocator{};
    };

}