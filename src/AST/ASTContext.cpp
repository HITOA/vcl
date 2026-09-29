#include <VCL/AST/ASTContext.hpp>

#include <VCL/AST/Decl.hpp>


VCL::ASTContext::ASTContext(TypeCache& typecache) 
        : nodeAllocator{}, destructors{}, typeCache{}, root{ AllocateNode<TranslationUnitDecl>() } {
    typecache.InsertTypeCacheChild(&this->typeCache);
}

VCL::ASTContext::~ASTContext() {
    for (auto it = destructors.rbegin(); it != destructors.rend(); ++it)
        it->second(it->first);
	typeCache.GetParent()->RemoveTypecacheChild(&typeCache);
}
const std::string& VCL::ASTContext::GetMangledName(NamedDecl* decl) {
    auto it = mangledNames.find(decl);
    if (it != mangledNames.end())
        return it->second;

    std::string name = manglingPrefix + "." + decl->GetIdentifierInfo()->GetName().str();
    uint32_t& uses = mangledNameUses[name];
    if (uses > 0)
        name += "." + std::to_string(uses);
    ++uses;
    return mangledNames.insert({ decl, std::move(name) }).first->second;
}
