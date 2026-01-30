#include <VCL/Core/Identifier.hpp>



VCL::IdentifierInfo* VCL::IdentifierTable::GetKeyword(TokenKind kind) {
    for (auto& identifier : identifiers)
        if (identifier.getValue()->GetTokenKind() == kind)
            return identifier.getValue();
    return nullptr;
}

void VCL::IdentifierTable::AddKeyword(llvm::StringRef name, TokenKind kind) {
    IdentifierInfo* info = Get(name);
    info->isKeyword = true;
    info->kind = kind;
}

void VCL::IdentifierTable::AddKeywords() {
    #define KEYWORD(x) AddKeyword(#x, TokenKind::Keyword_##x);
    #include <VCL/Core/TokenKind.def>
}