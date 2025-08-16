#pragma once


namespace VCL {
    
    /**
     * Simple enum for each error the Lexer class could return
     */
    enum class LexerError : unsigned short {
        None,
        #define LEXER_ERROR(x, y) x,
        #include <VCL/Core/Error.def>
        Max
    };

    /**
     * Provide a way to get a proper string for a given LexerError
     */
    inline const char* ToString(LexerError error) {
        switch (error) {
            #define LEXER_ERROR(x, y) case LexerError::x: return y;
            #include <VCL/Core/Error.def>
            default: return "Unknown error";
        }
    }

}