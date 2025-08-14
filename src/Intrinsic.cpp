#include "Intrinsic.hpp"

#include "ModuleContext.hpp"
#include "Utility.hpp"

#include <iostream>


namespace VCL {
    bool IntrinsicArgumentPolicy::operator()(Type type) {
        std::shared_ptr<TypeInfo> typeInfo = type.GetTypeInfo();

        if (policy & Numeric) {
            switch (typeInfo->type) {
            case TypeInfo::TypeName::Float:
            case TypeInfo::TypeName::Int:
            case TypeInfo::TypeName::Double:
                return true;
            }
        }

        if (policy & Vector) {
            switch (typeInfo->type) {
            case TypeInfo::TypeName::VectorFloat:
            case TypeInfo::TypeName::VectorInt:
            case TypeInfo::TypeName::VectorDouble:
                return true;
            }
        }

        if (policy & Condition) {
            switch (typeInfo->type)
            {
            case TypeInfo::TypeName::Bool:
                return true;
            }
        }

        if (policy & Mask) {
            switch (typeInfo->type)
            {
            case TypeInfo::TypeName::VectorBool:
                return true;
            }
        }

        if (policy & Buffer) {
            switch (typeInfo->type)
            {
            case TypeInfo::TypeName::Array:
            case TypeInfo::TypeName::Span:
                return true;
            }
        }

        if (policy & FloatOnly) {
            switch (typeInfo->type)
            {
            case TypeInfo::TypeName::Float:
            case TypeInfo::TypeName::VectorFloat:
            case TypeInfo::TypeName::Double:
            case TypeInfo::TypeName::VectorDouble:
                return true;
            }
        }

        return false;
    }

    class UnaryIntrinsicImpl : public IntrinsicImpl {
    public:
        UnaryIntrinsicImpl(llvm::Intrinsic::ID intrinsicId): intrinsicId{ intrinsicId } {};

        std::expected<Handle<Value>, Error> Call(std::vector<Handle<Value>>& argsv, ModuleContext* context) override {
            llvm::Value* r = context->GetIRBuilder().CreateUnaryIntrinsic(intrinsicId, argsv[0]->GetLLVMValue());
            return Value::Create(r, argsv[0]->GetType(), context);
        }
        
        bool CheckArgType(uint32_t index, Type type) override {
            IntrinsicArgumentPolicy policy{ IntrinsicArgumentPolicy::FloatOnly };
            return policy(type);
        }

        bool CheckArgCount(uint32_t count) override {
            return count == 1;
        }

    private:
        llvm::Intrinsic::ID intrinsicId;
    };

    class BinaryIntrinsicImpl : public IntrinsicImpl {
    public:
        BinaryIntrinsicImpl(llvm::Intrinsic::ID intrinsicId): intrinsicId{ intrinsicId } {};

        std::expected<Handle<Value>, Error> Call(std::vector<Handle<Value>>& argsv, ModuleContext* context) override {
            Type type = argsv[0]->GetType();
            std::vector<Handle<Value>> castedArgsv( argsv.size() );
            for (size_t i = 0; i < castedArgsv.size(); ++i) {
                auto r = argsv[i]->Cast(type);
                if (r.has_value())
                    castedArgsv[i] = *r;
                else
                    return std::unexpected(r.error());
            }
            llvm::Value* r = context->GetIRBuilder().CreateBinaryIntrinsic(intrinsicId, 
                castedArgsv[0]->GetLLVMValue(), castedArgsv[1]->GetLLVMValue());
            return Value::Create(r, argsv[0]->GetType(), context);
        }
        
        bool CheckArgType(uint32_t index, Type type) override {
            IntrinsicArgumentPolicy policy{ IntrinsicArgumentPolicy::FloatOnly };
            return policy(type);
        }

        bool CheckArgCount(uint32_t count) override {
            return count == 2;
        }

    private:
        llvm::Intrinsic::ID intrinsicId;
    };

    class TrinaryIntrinsicImpl : public IntrinsicImpl {
    public:
        TrinaryIntrinsicImpl(llvm::Intrinsic::ID intrinsicId): intrinsicId{ intrinsicId } {};

        std::expected<Handle<Value>, Error> Call(std::vector<Handle<Value>>& argsv, ModuleContext* context) override {
            Type type = argsv[0]->GetType();
            std::vector<Handle<Value>> castedArgsv( argsv.size() );
            for (size_t i = 0; i < castedArgsv.size(); ++i) {
                auto r = argsv[i]->Cast(type);
                if (r.has_value())
                    castedArgsv[i] = *r;
                else
                    return std::unexpected(r.error());
            }
            llvm::Value* r = context->GetIRBuilder().CreateIntrinsic(intrinsicId, { castedArgsv[0]->GetType().GetLLVMType() },
                { castedArgsv[0]->GetLLVMValue(), castedArgsv[1]->GetLLVMValue(), castedArgsv[2]->GetLLVMValue() });
            return Value::Create(r, argsv[0]->GetType(), context);
        }
        
        bool CheckArgType(uint32_t index, Type type) override {
            IntrinsicArgumentPolicy policy{ IntrinsicArgumentPolicy::FloatOnly };
            return policy(type);
        }

        bool CheckArgCount(uint32_t count) override {
            return count == 3;
        }

    private:
        llvm::Intrinsic::ID intrinsicId;
    };
}

VCL::Intrinsic::Intrinsic(std::unique_ptr<IntrinsicImpl> impl, Type type, ModuleContext* context) : Callable{
    nullptr, type, context
}, impl{ std::move(impl) } {

}

std::expected<VCL::Handle<VCL::Value>, VCL::Error> VCL::Intrinsic::Call(std::vector<Handle<Value>>& argsv) {
    for (size_t i = 0; i < argsv.size(); ++i) {
        if (!IsArgGivenByReference(i)) {
            if (auto loadedArgv = argsv[i]->Load(); loadedArgv.has_value())
                argsv[i] = *loadedArgv;
            else
                return std::unexpected{ loadedArgv.error() };
        }
    }
    return impl->Call(argsv, GetModuleContext());
}

bool VCL::Intrinsic::CheckArgType(uint32_t index, Type type) {
    return impl->CheckArgType(index, type);
}

bool VCL::Intrinsic::CheckArgCount(uint32_t count) {
    return impl->CheckArgCount(count);
}

bool VCL::Intrinsic::IsArgGivenByReference(uint32_t index) {
    return impl->IsArgGivenByReference(index);
}

VCL::CallableType VCL::Intrinsic::GetCallableType() {
    return CallableType::Intrinsic;
}

std::expected<VCL::Handle<VCL::Intrinsic>, VCL::Error> VCL::Intrinsic::Create(std::unique_ptr<IntrinsicImpl> impl, ModuleContext* context) {
    std::shared_ptr<TypeInfo> callableTypeInfo = std::make_shared<TypeInfo>();
    callableTypeInfo->type = TypeInfo::TypeName::Callable;
    if (auto type = Type::Create(callableTypeInfo, context); type.has_value())
        return MakeHandle<Intrinsic>(std::move(impl), *type, context);
    else
        return std::unexpected(type.error());
}

std::expected<VCL::Handle<VCL::Intrinsic>, VCL::Error> VCL::Intrinsic::CreateUnaryIntrinsic(llvm::Intrinsic::ID intrinsic, ModuleContext* context) {
    return Create(std::make_unique<UnaryIntrinsicImpl>(intrinsic), context);
}

std::expected<VCL::Handle<VCL::Intrinsic>, VCL::Error> VCL::Intrinsic::CreateBinaryIntrinsic(llvm::Intrinsic::ID intrinsic, ModuleContext* context) {
    return Create(std::make_unique<BinaryIntrinsicImpl>(intrinsic), context);
}

std::expected<VCL::Handle<VCL::Intrinsic>, VCL::Error> VCL::Intrinsic::CreateTrinaryIntrinsic(llvm::Intrinsic::ID intrinsic, ModuleContext* context) {
    return Create(std::make_unique<TrinaryIntrinsicImpl>(intrinsic), context);
}