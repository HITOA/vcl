#include "Intrinsics.hpp"

#include "ModuleContext.hpp"
#include "Utility.hpp"

#include <VCL/Debug.hpp>

#include <format>

#define DEFINE_UNARY_INTRINSIC(name, intrinsic) sm.PushNamedValue(name, ThrowOnErrorRE(Intrinsic::CreateUnaryIntrinsic(intrinsic, context)))
#define DEFINE_BINARY_INTRINSIC(name, intrinsic) sm.PushNamedValue(name, ThrowOnErrorRE(Intrinsic::CreateBinaryIntrinsic(intrinsic, context)))
#define DEFINE_TRINARY_INTRINSIC(name, intrinsic) sm.PushNamedValue(name, ThrowOnErrorRE(Intrinsic::CreateTrinaryIntrinsic(intrinsic, context)))


namespace VCL {
    
    class FModIntrinsicImpl : public IntrinsicImpl {
    public:
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
            llvm::Value* r = context->GetIRBuilder().CreateFRem(castedArgsv[0]->GetLLVMValue(), castedArgsv[0]->GetLLVMValue());
            return MakeValueVCLFromLLVM(r, context);
        }
        
        bool CheckArgType(uint32_t index, Type type) override {
            IntrinsicArgumentPolicy policy{ IntrinsicArgumentPolicy::Numeric | IntrinsicArgumentPolicy::Vector };
            return policy(type);
        }

        bool CheckArgCount(uint32_t count) override {
            return count == 2;
        }
    private:
        llvm::Intrinsic::ID intrinsicId;
    };

    class SelectIntrinsicImpl : public IntrinsicImpl {
    public:
        std::expected<Handle<Value>, Error> Call(std::vector<Handle<Value>>& argsv, ModuleContext* context) override {
            Handle<Value> condition = argsv[0];
            
            Handle<Value> v1;
            
            if (auto r = argsv[1]->Splat(); r.has_value())
                v1 = *r;
            else
                return std::unexpected(r.error());
            
            if (auto v2 = argsv[2]->Cast(v1->GetType()); v2.has_value()) {
                llvm::Value* r = context->GetIRBuilder().CreateSelect(condition->GetLLVMValue(), 
                    v1->GetLLVMValue(), (*v2)->GetLLVMValue());
                return MakeValueVCLFromLLVM(r, context);
            } else
                return std::unexpected(v2.error());
        }
        
        bool CheckArgType(uint32_t index, Type type) override {
            IntrinsicArgumentPolicy policy{ IntrinsicArgumentPolicy::Numeric | IntrinsicArgumentPolicy::Vector };
            if (index == 0)
                policy.policy = IntrinsicArgumentPolicy::Mask;
            return policy(type);
        }

        bool CheckArgCount(uint32_t count) override {
            return count == 3;
        }
    private:
        llvm::Intrinsic::ID intrinsicId;
    };

    class LenIntrinsicImpl : public IntrinsicImpl {
    public:
        std::expected<Handle<Value>, Error> Call(std::vector<Handle<Value>>& argsv, ModuleContext* context) override {
            Handle<Value> value = argsv[0];
            if (value->GetType().GetTypeInfo()->type == TypeInfo::TypeName::Array) {
                llvm::ArrayType* arrayType = llvm::cast<llvm::ArrayType>(value->GetType().GetLLVMType());
                return Value::CreateConstantInt32(arrayType->getNumElements(), context);
            } else if (value->GetType().GetTypeInfo()->type == TypeInfo::TypeName::Span) {
                llvm::StructType* structType = llvm::cast<llvm::StructType>(value->GetType().GetLLVMType());
                llvm::Value* idx = context->GetIRBuilder().CreateExtractValue(value->GetLLVMValue(), 1);
                return MakeValueVCLFromLLVM(idx, context);
            } else {
                return std::unexpected{ Error{ std::format("Cannot take length of `{}`.", ToString(value->GetType().GetTypeInfo())) } };
            }
        }
        
        bool CheckArgType(uint32_t index, Type type) override {
            IntrinsicArgumentPolicy policy{ IntrinsicArgumentPolicy::Buffer};
            return policy(type);
        }

        bool CheckArgCount(uint32_t count) override {
            return count == 1;
        }
    private:
        llvm::Intrinsic::ID intrinsicId;
    };

}

template<typename T>
inline T ThrowOnErrorRE(std::expected<T, VCL::Error> value) {
    if (value.has_value())
        return *value;
    throw std::runtime_error{ value.error() };
}

void VCL::Intrinsics::Register(ModuleContext* context) {
    ScopeManager& sm = context->GetScopeManager();

    DEFINE_UNARY_INTRINSIC("sqrt",      llvm::Intrinsic::sqrt);
    DEFINE_UNARY_INTRINSIC("sin",       llvm::Intrinsic::sin);
    DEFINE_UNARY_INTRINSIC("cos",       llvm::Intrinsic::cos);
    DEFINE_UNARY_INTRINSIC("tan",       llvm::Intrinsic::tan);
    DEFINE_UNARY_INTRINSIC("asin",      llvm::Intrinsic::asin);
    DEFINE_UNARY_INTRINSIC("acos",      llvm::Intrinsic::acos);
    DEFINE_UNARY_INTRINSIC("atan",      llvm::Intrinsic::atan);
    DEFINE_UNARY_INTRINSIC("sinh",      llvm::Intrinsic::sinh);
    DEFINE_UNARY_INTRINSIC("cosh",      llvm::Intrinsic::cosh);
    DEFINE_UNARY_INTRINSIC("tanh",      llvm::Intrinsic::tanh);
    DEFINE_UNARY_INTRINSIC("log",       llvm::Intrinsic::log);
    DEFINE_UNARY_INTRINSIC("log10",     llvm::Intrinsic::log10);
    DEFINE_UNARY_INTRINSIC("log2",      llvm::Intrinsic::log2);
    DEFINE_UNARY_INTRINSIC("exp",       llvm::Intrinsic::exp);
    DEFINE_UNARY_INTRINSIC("exp10",     llvm::Intrinsic::exp10);
    DEFINE_UNARY_INTRINSIC("exp2",      llvm::Intrinsic::exp2);
    DEFINE_UNARY_INTRINSIC("fabs",      llvm::Intrinsic::fabs);
    DEFINE_UNARY_INTRINSIC("ceil",      llvm::Intrinsic::ceil);
    DEFINE_UNARY_INTRINSIC("floor",     llvm::Intrinsic::floor);
    DEFINE_UNARY_INTRINSIC("round",     llvm::Intrinsic::round);
    sm.PushNamedValue("len", ThrowOnErrorRE(Intrinsic::Create(std::make_unique<LenIntrinsicImpl>(), context)));

    DEFINE_BINARY_INTRINSIC("pow",      llvm::Intrinsic::pow);
    DEFINE_BINARY_INTRINSIC("min",      llvm::Intrinsic::minnum);
    DEFINE_BINARY_INTRINSIC("max",      llvm::Intrinsic::maxnum);
    sm.PushNamedValue("fmod", ThrowOnErrorRE(Intrinsic::Create(std::make_unique<FModIntrinsicImpl>(), context)));

    DEFINE_TRINARY_INTRINSIC("fma",     llvm::Intrinsic::fma);
    DEFINE_TRINARY_INTRINSIC("brev",    llvm::Intrinsic::bitreverse);
    sm.PushNamedValue("select", ThrowOnErrorRE(Intrinsic::Create(std::make_unique<SelectIntrinsicImpl>(), context)));
}