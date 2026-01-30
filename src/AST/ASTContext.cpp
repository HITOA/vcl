#include <VCL/AST/ASTContext.hpp>

#include <VCL/AST/Decl.hpp>


VCL::ASTContext::ASTContext(TypeCache& typecache) 
        : nodeAllocator{}, typeCache{ typecache }, root{ AllocateNode<TranslationUnitDecl>() } {}