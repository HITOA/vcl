#pragma once

#include <memory>


namespace VCL {

    template<typename T>
    using Handle = std::shared_ptr<T>;

    template<typename T, typename... Args>
    inline Handle<T> MakeHandle(Args&&... args) {
        return std::make_shared<T>(std::forward<Args>(args)...);
    }

}