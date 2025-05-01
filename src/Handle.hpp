#pragma once

#include <memory>


namespace VCL {
    template<typename T>
    using Handle = std::shared_ptr<T>;
    
    template<typename T, typename... Args>
    inline auto MakeHandle(Args&&... args) { return std::make_shared<T>(std::forward<Args>(args)...); }

    template<typename T, typename U>
    inline auto HandleCast(std::shared_ptr<U>& r) { return std::static_pointer_cast<T, U>(r); }
}