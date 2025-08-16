#pragma once


namespace VCL {
    
    /**
     * Simple enum for each error the Source class could return
     */
    enum class SourceError : unsigned short {
        #define SOURCE_ERROR(x, y) x,
        #include <VCL/Core/Error.def>
        Max
    };

    /**
     * Provide a way to get a proper string for a given SourceError
     */
    inline const char* ToString(SourceError error) {
        switch (error) {
            #define SOURCE_ERROR(x, y) case SourceError::x: return y;
            #include <VCL/Core/Error.def>
            default: return "Unknown error";
        }
    }

}