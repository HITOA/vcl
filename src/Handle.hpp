#pragma once

#include <memory>


namespace VCL {
    template<typename T>
    using Handle = std::shared_ptr<T>;
    
    template<typename T, typename... Args>
    using MakeHandle = std::make_shared<T, Args...>;

    template<typename T, typename U>
    using HandleCast = std::static_pointer_cast<T, U>;
}