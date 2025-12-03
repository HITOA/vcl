#include <VCL/CodeGen/CodeGenModule.hpp>

#include <llvm/IR/Constants.h>


llvm::Constant* VCL::CodeGenModule::GenerateConstantValue(ConstantValue* value) {
    switch (value->GetConstantValueClass()) {
        case ConstantValue::ConstantScalarClass:
            return GenerateConstantScalar((ConstantScalar*)value);
        case ConstantValue::ConstantAggregateClass:
            return GenerateConstantAggregate((ConstantAggregate*)value);
        case ConstantValue::ConstantNullClass:
            return GenerateConstantNull((ConstantNull*)value);
        default:
            diagnosticReporter.Error(Diagnostic::InternalError)
                .SetCompilerInfo(__FILE__, __func__, __LINE__)
                .Report();
            return nullptr;
    }
}

llvm::Constant* VCL::CodeGenModule::GenerateConstantScalar(ConstantScalar* scalar) {
    switch (scalar->GetKind()) {
        case BuiltinType::Float32:
            return llvm::ConstantFP::get(GetLLVMContext(), llvm::APFloat{ scalar->Get<float>() });
        case BuiltinType::Float64:
            return llvm::ConstantFP::get(GetLLVMContext(), llvm::APFloat{ scalar->Get<double>() });
        case BuiltinType::Int8:
            return llvm::ConstantInt::get(GetLLVMContext(), llvm::APInt{ 8, scalar->Get<uint64_t>(), true });
        case BuiltinType::Int16:
            return llvm::ConstantInt::get(GetLLVMContext(), llvm::APInt{ 16, scalar->Get<uint64_t>(), true });
        case BuiltinType::Int32:
            return llvm::ConstantInt::get(GetLLVMContext(), llvm::APInt{ 32, scalar->Get<uint64_t>(), true });
        case BuiltinType::Int64:
            return llvm::ConstantInt::get(GetLLVMContext(), llvm::APInt{ 64, scalar->Get<uint64_t>(), true });
        case BuiltinType::UInt8:
            return llvm::ConstantInt::get(GetLLVMContext(), llvm::APInt{ 8, scalar->Get<uint64_t>(), false });
        case BuiltinType::UInt16:
            return llvm::ConstantInt::get(GetLLVMContext(), llvm::APInt{ 16, scalar->Get<uint64_t>(), false });
        case BuiltinType::UInt32:
            return llvm::ConstantInt::get(GetLLVMContext(), llvm::APInt{ 32, scalar->Get<uint64_t>(), false });
        case BuiltinType::UInt64:
            return llvm::ConstantInt::get(GetLLVMContext(), llvm::APInt{ 64, scalar->Get<uint64_t>(), false });
        default:
            diagnosticReporter.Error(Diagnostic::InternalError)
                .SetCompilerInfo(__FILE__, __func__, __LINE__)
                .Report();
            return nullptr;
    }
}

llvm::Constant* VCL::CodeGenModule::GenerateConstantAggregate(ConstantAggregate* aggregate) {
    llvm::SmallVector<llvm::Constant*> values{};
    values.reserve(aggregate->GetValueCount());

    for (ConstantValue* value : aggregate->GetValues()) {
        llvm::Constant* constantValue = GenerateConstantValue(value);
        if (!constantValue) {
            diagnosticReporter.Error(Diagnostic::InternalError)
                .SetCompilerInfo(__FILE__, __func__, __LINE__)
                .Report();
            return nullptr;
        }
        values.push_back(constantValue);
    }

    Type* type = Type::GetCanonicalType(aggregate->GetType().GetType());
    llvm::Type* llvmType = cgt.ConvertType(type);

    switch (type->GetTypeClass()) {
        case Type::ArrayTypeClass: {
            return llvm::ConstantArray::get(llvm::cast<llvm::ArrayType>(llvmType), values);
        }
        case Type::RecordTypeClass: {
            return llvm::ConstantStruct::get(llvm::cast<llvm::StructType>(llvmType), values);
        }
        default:
            diagnosticReporter.Error(Diagnostic::InternalError)
                .SetCompilerInfo(__FILE__, __func__, __LINE__)
                .Report();
            return nullptr;
    }
}

llvm::Constant* VCL::CodeGenModule::GenerateConstantNull(ConstantNull* null) {
    llvm::Type* type = cgt.ConvertType(null->GetType());
    return llvm::Constant::getNullValue(type);
}