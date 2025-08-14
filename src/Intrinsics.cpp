#include "Intrinsics.hpp"

#include "ModuleContext.hpp"
#include "Utility.hpp"

#include <VCL/Debug.hpp>
#include <VCL/NativeTarget.hpp>

#include <format>
#include <iostream>

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
            return Value::Create(r, argsv[0]->GetType(), context);
        }
        
        bool CheckArgType(uint32_t index, Type type) override {
            IntrinsicArgumentPolicy policy{ IntrinsicArgumentPolicy::Numeric | IntrinsicArgumentPolicy::Vector };
            return policy(type);
        }

        bool CheckArgCount(uint32_t count) override {
            return count == 2;
        }
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
                return Value::Create(r, v1->GetType(), context);
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
                std::shared_ptr<TypeInfo> typeInfo = std::make_shared<TypeInfo>();
                typeInfo->type = TypeInfo::TypeName::Int;
                if (auto t = Type::Create(typeInfo, context); t.has_value())
                    return Value::Create(idx, *t, context);
                else
                    return std::unexpected{ t.error() };
            } else if (value->GetType().GetTypeInfo()->IsVector()) {
                uint32_t vectorElementWidth = NativeTarget::GetInstance()->GetMaxVectorElementWidth();
                return Value::CreateConstantInt32((int)vectorElementWidth, context);
            } else {
                return std::unexpected{ Error{ std::format("Cannot take length of `{}`.", ToString(value->GetType().GetTypeInfo())) } };
            }
        }
        
        bool CheckArgType(uint32_t index, Type type) override {
            IntrinsicArgumentPolicy policy{ IntrinsicArgumentPolicy::Buffer | IntrinsicArgumentPolicy::Vector };
            return policy(type);
        }

        bool CheckArgCount(uint32_t count) override {
            return count == 1;
        }
    };

    class ExtractIntrinsicImpl : public IntrinsicImpl {
    public:
        std::expected<Handle<Value>, Error> Call(std::vector<Handle<Value>>& argsv, ModuleContext* context) override {
            Handle<Value> vector = argsv[0];
            Handle<Value> index = argsv[1];
            std::shared_ptr<TypeInfo> typeInfo = std::make_shared<TypeInfo>();
            typeInfo->type = GetScalarTypeName(vector->GetType().GetTypeInfo()->type);
            if (auto t = Type::Create(typeInfo, context); t.has_value())
                return Value::Create(context->GetIRBuilder().CreateExtractElement(
                    vector->GetLLVMValue(), index->GetLLVMValue()), *t, context);
            else
                return std::unexpected{ t.error() };
        }
        
        bool CheckArgType(uint32_t index, Type type) override {
            if (index == 0)
                return IntrinsicArgumentPolicy{ IntrinsicArgumentPolicy::Vector }(type);
            return type.GetTypeInfo()->type == TypeInfo::TypeName::Int;
        }

        bool CheckArgCount(uint32_t count) override {
            return count == 2;
        }
    };

    class InsertIntrinsicImpl : public IntrinsicImpl {
    public:
        std::expected<Handle<Value>, Error> Call(std::vector<Handle<Value>>& argsv, ModuleContext* context) override {
            Handle<Value> vector = argsv[0];
            Handle<Value> index = argsv[1];
            Handle<Value> value = argsv[2];

            if (value->GetType().GetTypeInfo()->type != GetScalarTypeName(vector->GetType().GetTypeInfo()->type))
                return std::unexpected{ VCL::Error{ std::format("Cannot insert an element of type `{}` into a vector of type `{}`",
                    ToString(value->GetType().GetTypeInfo()), ToString(vector->GetType().GetTypeInfo())) } };
            if (auto r = vector->Load(); r.has_value()) {
                llvm::Value* inserted = context->GetIRBuilder().CreateInsertElement((*r)->GetLLVMValue(), value->GetLLVMValue(), index->GetLLVMValue());
                context->GetIRBuilder().CreateStore(inserted, vector->GetLLVMValue());
                return vector;
            } else {
                return std::unexpected{ r.error() };
            }
        }
        
        bool CheckArgType(uint32_t index, Type type) override {
            if (index == 0)
                return IntrinsicArgumentPolicy{ IntrinsicArgumentPolicy::Vector }(type);
            else if (index == 1)
                return type.GetTypeInfo()->type == TypeInfo::TypeName::Int;
            return IntrinsicArgumentPolicy{ IntrinsicArgumentPolicy::Numeric }(type);
        }

        bool CheckArgCount(uint32_t count) override {
            return count == 3;
        }

        bool IsArgGivenByReference(uint32_t index) override { 
            if (index == 0)
                return true;
            return false; 
        };
    };

    class StepIntrinsicImpl : public IntrinsicImpl {
    public:
        std::expected<Handle<Value>, Error> Call(std::vector<Handle<Value>>& argsv, ModuleContext* context) override {
            std::shared_ptr<TypeInfo> typeInfo = std::make_shared<TypeInfo>();
            typeInfo->type = TypeInfo::TypeName::VectorInt;
            if (auto t = Type::Create(typeInfo, context); t.has_value()) {
                return Value::Create(context->GetIRBuilder().CreateStepVector(t->GetLLVMType()), *t, context);
            } else {
                return std::unexpected{ t.error() };
            }
        }
        
        bool CheckArgType(uint32_t index, Type type) override {
            return true;
        }

        bool CheckArgCount(uint32_t count) override {
            return count == 0;
        }
    };

    class ReverseIntrinsicImpl : public IntrinsicImpl {
    public:
        std::expected<Handle<Value>, Error> Call(std::vector<Handle<Value>>& argsv, ModuleContext* context) override {
            Handle<Value> vector = argsv[0];
            std::shared_ptr<TypeInfo> typeInfo = std::make_shared<TypeInfo>();
            typeInfo->type = vector->GetType().GetTypeInfo()->type;
            if (auto t = Type::Create(typeInfo, context); t.has_value()) {
                return Value::Create(context->GetIRBuilder().CreateVectorReverse(vector->GetLLVMValue()), *t, context);
            } else {
                return std::unexpected{ t.error() };
            }
        }
        
        bool CheckArgType(uint32_t index, Type type) override {
            return IntrinsicArgumentPolicy{ IntrinsicArgumentPolicy::Vector }(type);
        }

        bool CheckArgCount(uint32_t count) override {
            return count == 1;
        }
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

    sm.PushNamedValue("step", ThrowOnErrorRE(Intrinsic::Create(std::make_unique<StepIntrinsicImpl>(), context)));

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
    //DEFINE_UNARY_INTRINSIC("exp10",     llvm::Intrinsic::exp10);
    DEFINE_UNARY_INTRINSIC("exp2",      llvm::Intrinsic::exp2);
    DEFINE_UNARY_INTRINSIC("fabs",      llvm::Intrinsic::fabs);
    DEFINE_UNARY_INTRINSIC("ceil",      llvm::Intrinsic::ceil);
    DEFINE_UNARY_INTRINSIC("floor",     llvm::Intrinsic::floor);
    DEFINE_UNARY_INTRINSIC("round",     llvm::Intrinsic::round);
    sm.PushNamedValue("len", ThrowOnErrorRE(Intrinsic::Create(std::make_unique<LenIntrinsicImpl>(), context)));
    sm.PushNamedValue("reverse", ThrowOnErrorRE(Intrinsic::Create(std::make_unique<ReverseIntrinsicImpl>(), context)));
    
    DEFINE_BINARY_INTRINSIC("pow",      llvm::Intrinsic::pow);
    sm.PushNamedValue("fmod", ThrowOnErrorRE(Intrinsic::Create(std::make_unique<FModIntrinsicImpl>(), context)));
    sm.PushNamedValue("extract", ThrowOnErrorRE(Intrinsic::Create(std::make_unique<ExtractIntrinsicImpl>(), context)));

    DEFINE_TRINARY_INTRINSIC("fma",     llvm::Intrinsic::fma);
    sm.PushNamedValue("select", ThrowOnErrorRE(Intrinsic::Create(std::make_unique<SelectIntrinsicImpl>(), context)));
    sm.PushNamedValue("insert", ThrowOnErrorRE(Intrinsic::Create(std::make_unique<InsertIntrinsicImpl>(), context)));
}