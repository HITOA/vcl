#pragma once

#include <memory>


namespace VCL {

    class NativeTarget {
    public:
        NativeTarget();
        ~NativeTarget();

        uint32_t GetMaxVectorByteWidth();
        uint32_t GetMaxVectorElementWidth();

        static std::shared_ptr<NativeTarget> GetInstance();
    };

}