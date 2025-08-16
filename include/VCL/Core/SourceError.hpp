#pragma once


namespace VCL {
    
    enum class SourceError : unsigned short {
        #define SOURCE_ERROR(x, y) x,
        #include <VCL/Core/Error.def>
        #undef SOURCE_ERROR
        Max
    };

    inline const char* ToString(SourceError error) {
        switch (error) {
            #define SOURCE_ERROR(x, y) case SourceError::x: return y;
            #include <VCL/Core/Error.def>
            #undef SOURCE_ERROR
            default: return "Unknown error";
        }
    }

}