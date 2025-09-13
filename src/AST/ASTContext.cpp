#include <VCL/AST/ASTContext.hpp>

#include <VCL/AST/Decl.hpp>


VCL::ASTContext::ASTContext() : nodeAllocator{}, typeCache{}, root{ AllocateNode<TranslationUnitDecl>() } {}