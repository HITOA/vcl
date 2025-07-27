#include <VCL/Directive.hpp>


std::string VCL::ImportDirective::GetDirectiveName() {
    return "import";
}

std::unique_ptr<VCL::ASTDirective> VCL::ImportDirective::Parse(Lexer& lexer, Parser* parser) {
    return nullptr;
}

std::string VCL::DefineDirective::GetDirectiveName() {
    return "define";
}

std::unique_ptr<VCL::ASTDirective> VCL::DefineDirective::Parse(Lexer& lexer, Parser* parser) {
    return nullptr;
}

std::string VCL::MacroDirective::GetDirectiveName() {
    return "macro";
}

std::unique_ptr<VCL::ASTDirective> VCL::MacroDirective::Parse(Lexer& lexer, Parser* parser) {
    return nullptr;
}