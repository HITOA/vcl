#include "Aggregate.hpp"

#include <VCL/Debug.hpp>

static VCL::Type GetAggregateType() {
    static std::shared_ptr<VCL::TypeInfo> typeInfo{};
    if (!typeInfo) {
        typeInfo = std::make_shared<VCL::TypeInfo>();
        typeInfo->type = VCL::TypeInfo::TypeName::Aggregate;
    }

    return VCL::Type{ typeInfo, nullptr, nullptr, nullptr, false };
} 

VCL::Aggregate::Aggregate(const std::vector<Handle<Value>>& values, bool isAllConst, ModuleContext* context) : 
    Value{ nullptr, GetAggregateType(), context }, values{ values }, isAllConst{ isAllConst } {}

std::expected<VCL::Handle<VCL::Value>, VCL::Error> VCL::Aggregate::Load() {
    return Create(values, context);
}

std::optional<VCL::Error> VCL::Aggregate::Store(Handle<Value> value) {
    return Error{ "Cannot store into an aggregate value." };
}

std::expected<VCL::Handle<VCL::Value>, VCL::Error> VCL::Aggregate::Splat() {
    return std::unexpected{ Error{ "Cannot splat an aggregate value." } };
}

std::expected<VCL::Handle<VCL::Value>, VCL::Error> VCL::Aggregate::Cast(Type type) {
    if (type.GetTypeInfo()->type != TypeInfo::TypeName::Array && type.GetTypeInfo()->type != TypeInfo::TypeName::Custom)
        return std::unexpected{ Error{ std::format("Cannot cast aggregate value to `{}`.", ToString(type.GetTypeInfo())) } };

    if (isAllConst) {
        std::vector<llvm::Constant*> elements( values.size() );
        for (size_t i = 0; i < values.size(); ++i)
            elements[i] = llvm::cast<llvm::Constant>(values[i]->GetLLVMValue());

        if (type.GetTypeInfo()->type == TypeInfo::TypeName::Array) {
            if (!llvm::isa<llvm::ArrayType>(type.GetLLVMType()))
                return std::unexpected{ Error{ "Unexpected LLVM type when casting." } };
            llvm::ArrayType* arrayType = llvm::cast<llvm::ArrayType>(type.GetLLVMType());
            llvm::Value* value = llvm::ConstantArray::get(arrayType, elements);
            return Value::Create(value, type, context);
        } else if (type.GetTypeInfo()->type == TypeInfo::TypeName::Custom) {
            if (!llvm::isa<llvm::StructType>(type.GetLLVMType()))
                return std::unexpected{ Error{ "Unexpected LLVM type when casting." } };
            llvm::StructType* structType = llvm::cast<llvm::StructType>(type.GetLLVMType());
            llvm::Value* value = llvm::ConstantStruct::get(structType, elements);
            return Value::Create(value, type, context);
        }
    }

    return std::unexpected{ Error{ "Aggregate cast not implemented." } };
}

std::expected<VCL::Handle<VCL::Aggregate>, VCL::Error> VCL::Aggregate::Create(const std::vector<Handle<Value>>& values, ModuleContext* context) {
    bool isAllConst = true;
    for (size_t i = 0; i < values.size(); ++i) {
        if (!llvm::isa<llvm::Constant>(values[i]->GetLLVMValue())) {
            isAllConst = false;
            break;
        }
    }
    return MakeHandle<Aggregate>(values, isAllConst, context);
}