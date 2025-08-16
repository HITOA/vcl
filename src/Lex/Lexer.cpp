#include <VCL/Lex/Lexer.hpp>



VCL::Lexer::Lexer(const llvm::MemoryBufferRef& buffer) {
    range.start = (uintptr_t)buffer.getBufferStart();
    range.end = (uintptr_t)buffer.getBufferEnd();
    currentLocation = range.start;
}

VCL::Lexer::Lexer(const SourceRange& range) : range{ range } {
    currentLocation = range.start;
}

bool VCL::Lexer::Lex(Token& token) {
    
}