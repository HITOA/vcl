#pragma once

#include <VCL/Core/TokenKind.hpp>

#include <llvm/ADT/StringRef.h>


namespace VCL {

    /**
     * Given a StringRef, return its corresponding keyword TokenKind.
     * If the given value does not correspond to any TokenKind,
     * TokenKind::Unknown will be returned instead.
     */
    TokenKind TokenKindLookupKeyword(llvm::StringRef value);
    
    /**
     * Given a StringRef, return its corresponding punctuator TokenKind.
     * If the given value does not correspond to any TokenKind,
     * TokenKind::Unknown will be returned instead.
     */
    TokenKind TokenKindLookupPunctuator(llvm::StringRef value);

}