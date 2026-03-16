#include <VCL/AST/ASTConsumer.hpp>



void VCL::MultiplexerASTConsumer::HandleTopLevelDecl(Decl* decl) {
    for (ASTConsumer* consumer : consumers)
        consumer->HandleTopLevelDecl(decl);
}