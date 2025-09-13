#pragma once


namespace VCL {

    template<typename RetTy>
    class VisitorTrait { 
    public:
        using ReturnType = RetTy;
    };

    #define DEFINE_HAS_VISIT_FUNC(Func) static constexpr bool Has##Func(decltype(&T::Func)) { return true; }\
        template<typename> static constexpr bool Has##Func(...) { return false; }

    #define DEFINE_TRY_CALL_VISIT_FUNC(Func) DEFINE_HAS_VISIT_FUNC(Func) \
        template<typename... Args> typename T::ReturnType TryCall##Func(Args&&... args) {\
            if constexpr (Has##Func(0)) return ((T*)this)->Func(std::forward<Args>(args)...);\
            return typename T::ReturnType{};\
        }

}