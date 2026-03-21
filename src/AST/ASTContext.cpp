#include <VCL/AST/ASTContext.hpp>

#include <VCL/AST/Decl.hpp>


VCL::ASTContext::ASTContext(TypeCache& typecache) 
        : nodeAllocator{}, typeCache{}, root{ AllocateNode<TranslationUnitDecl>() } {
    typecache.InsertTypeCacheChild(&this->typeCache);
}

VCL::ASTContext::~ASTContext() {
	typeCache.GetParent()->RemoveTypecacheChild(&typeCache);
}