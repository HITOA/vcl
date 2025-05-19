#pragma once

#include <VCL/Definition.hpp>

#include "Callable.hpp"

#include <functional>


namespace VCL {
    struct IntrinsicArgumentPolicy {
        enum Policy {
            None = FLAG(0),
            Numeric = FLAG(1),
            Vector = FLAG(2),
            Condition = FLAG(3),
            Mask = FLAG(4),
            Buffer = FLAG(5),
            FloatOnly = FLAG(6)
        } policy = Policy::None;

        bool operator()(Type type);

        friend inline IntrinsicArgumentPolicy::Policy operator|(IntrinsicArgumentPolicy::Policy a, IntrinsicArgumentPolicy::Policy b) {
            return (IntrinsicArgumentPolicy::Policy)((int)a | (int)b);
        }

        friend inline IntrinsicArgumentPolicy::Policy operator|=(IntrinsicArgumentPolicy::Policy& a, IntrinsicArgumentPolicy::Policy b) {
            a = (IntrinsicArgumentPolicy::Policy)((int)a | (int)b);
            return a;
        }

        friend inline bool operator&(IntrinsicArgumentPolicy::Policy a, IntrinsicArgumentPolicy::Policy b) {
            return ((int)a & (int)b) != 0;
        }
    };

    class IntrinsicImpl {
    public:
        virtual ~IntrinsicImpl() {};
        virtual std::expected<Handle<Value>, Error> Call(std::vector<Handle<Value>>& argsv, ModuleContext* context) = 0;
        virtual bool CheckArgType(uint32_t index, Type type) = 0;
        virtual bool CheckArgCount(uint32_t count) = 0;
    };

    class Intrinsic : public Callable {
    public:
        Intrinsic() = delete;
        Intrinsic(std::unique_ptr<IntrinsicImpl> impl, Type type, ModuleContext* context);
        virtual ~Intrinsic() = default;

        Intrinsic& operator=(const Intrinsic& value) = default;
        Intrinsic& operator=(Intrinsic&& value) noexcept = default;
        
        std::expected<Handle<Value>, Error> Call(std::vector<Handle<Value>>& argsv) override;

        bool CheckArgType(uint32_t index, Type type) override;

        bool CheckArgCount(uint32_t count) override;

        CallableType GetCallableType() override;

        static std::expected<Handle<Intrinsic>, Error> Create(std::unique_ptr<IntrinsicImpl> impl, ModuleContext* context);
        static std::expected<Handle<Intrinsic>, Error> CreateUnaryIntrinsic(llvm::Intrinsic::ID intrinsic, ModuleContext* context);
        static std::expected<Handle<Intrinsic>, Error> CreateBinaryIntrinsic(llvm::Intrinsic::ID intrinsic, ModuleContext* context);
        static std::expected<Handle<Intrinsic>, Error> CreateTrinaryIntrinsic(llvm::Intrinsic::ID intrinsic, ModuleContext* context);

    private:
        std::unique_ptr<IntrinsicImpl> impl;
    };

}