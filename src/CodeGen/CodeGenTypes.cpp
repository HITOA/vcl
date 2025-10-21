#include <VCL/CodeGen/CodeGenTypes.hpp>

#include <VCL/CodeGen/CodeGenModule.hpp>


llvm::Type* VCL::CodeGenTypes::ConvertType(QualType type) {
    if (types.count(type.GetType()))
        return types.at(type.GetType());

    switch (type.GetType()->GetTypeClass()) {
        case Type::TemplateTypeParamTypeClass:
            cgm.GetCC().GetDiagnosticReporter().Error(Diagnostic::InternalError)
                .SetCompilerInfo(__FILE__, __func__, __LINE__)
                .Report();
            return nullptr;
        case Type::BuiltinTypeClass: return ConvertBuiltinType(type);
        case Type::VectorTypeClass: return ConvertVectorType(type);
        case Type::ArrayTypeClass: return ConvertArrayType(type);
        case Type::SpanTypeClass: return ConvertSpanType(type);
        case Type::RecordTypeClass: return ConvertRecordDeclType(type);
        case Type::FunctionTypeClass: return ConvertFunctionType(type);
        case Type::ReferenceTypeClass: return ConvertReferenceType(type);
        case Type::TemplateSpecializationTypeClass: {
            Type* instantiatedType = ((TemplateSpecializationType*)type.GetType())->GetInstantiatedType();
            if (!instantiatedType) {
                cgm.GetCC().GetDiagnosticReporter().Error(Diagnostic::InternalError)
                    .SetCompilerInfo(__FILE__, __func__, __LINE__)
                    .Report();
                return nullptr;
            }
            return ConvertType(instantiatedType);
        }
        default:
            cgm.GetCC().GetDiagnosticReporter().Error(Diagnostic::InternalError)
                .SetCompilerInfo(__FILE__, __func__, __LINE__)
                .Report();
            return nullptr;
    }
}

llvm::Type* VCL::CodeGenTypes::ConvertBuiltinType(QualType type) {
    BuiltinType* builtinType = (BuiltinType*)type.GetType();
    llvm::Type* resultType = nullptr;
    switch (builtinType->GetKind()) {
        case BuiltinType::Void: 
            resultType = llvm::Type::getVoidTy(cgm.GetLLVMContext());
            break;
        case BuiltinType::Bool: 
            resultType = llvm::Type::getInt1Ty(cgm.GetLLVMContext());
            break;
        case BuiltinType::UInt8:
        case BuiltinType::Int8: 
            resultType = llvm::Type::getInt8Ty(cgm.GetLLVMContext());
            break;
        case BuiltinType::UInt16:
        case BuiltinType::Int16: 
            resultType = llvm::Type::getInt16Ty(cgm.GetLLVMContext());
            break;
        case BuiltinType::UInt32:
        case BuiltinType::Int32: 
            resultType = llvm::Type::getInt32Ty(cgm.GetLLVMContext());
            break;
        case BuiltinType::UInt64:
        case BuiltinType::Int64: 
            resultType = llvm::Type::getInt64Ty(cgm.GetLLVMContext());
            break;
        case BuiltinType::Float32: 
            resultType = llvm::Type::getFloatTy(cgm.GetLLVMContext());
            break;
        case BuiltinType::Float64: 
            resultType = llvm::Type::getDoubleTy(cgm.GetLLVMContext());
            break;
        default:
            cgm.GetCC().GetDiagnosticReporter().Error(Diagnostic::InternalError)
                .SetCompilerInfo(__FILE__, __func__, __LINE__)
                .Report();
            return nullptr;
    }
    types.insert(std::make_pair(type.GetType(), resultType));
    return resultType;
}

llvm::VectorType* VCL::CodeGenTypes::ConvertVectorType(QualType type) {
    VectorType* vectorType = (VectorType*)type.GetType();
    llvm::Type* ofType = ConvertType(vectorType->GetElementType());
    if (!ofType)
        return nullptr;
    llvm::VectorType* resultType = llvm::FixedVectorType::get(ofType, cgm.GetTarget().GetVectorWidthInElement());
    types.insert(std::make_pair(type.GetType(), resultType));
    return resultType;
}

llvm::ArrayType* VCL::CodeGenTypes::ConvertArrayType(QualType type) {
    ArrayType* arrayType = (ArrayType*)type.GetType();
    llvm::Type* ofType = ConvertType(arrayType->GetElementType());
    if (!ofType)
        return nullptr;
    llvm::ArrayType* resultType = llvm::ArrayType::get(ofType, arrayType->GetElementCount());
    types.insert(std::make_pair(type.GetType(), resultType));
    return resultType;
}

llvm::StructType* VCL::CodeGenTypes::ConvertSpanType(QualType type) {
    SpanType* spanType = (SpanType*)type.GetType();
    llvm::Type* ofType = ConvertType(spanType->GetElementType());
    llvm::PointerType* ofTypePtr = llvm::PointerType::get(ofType, 0);
    llvm::Type* ofSizeType = llvm::Type::getInt64Ty(cgm.GetLLVMContext());
    if (!ofType)
        return nullptr;
    llvm::StructType* resultType = llvm::StructType::get(cgm.GetLLVMContext(), { ofTypePtr, ofSizeType });
    types.insert(std::make_pair(type.GetType(), resultType));
    return resultType;
}

llvm::StructType* VCL::CodeGenTypes::ConvertRecordDeclType(QualType type) {
    RecordDecl* decl = ((RecordType*)type.GetType())->GetRecordDecl();
    llvm::SmallVector<llvm::Type*> fields;
    for (auto it = decl->Begin(); it != decl->End(); ++it) {
        if (it->GetDeclClass() != Decl::FieldDeclClass)
            continue;
        FieldDecl* fieldDecl = (FieldDecl*)it.Get();
        llvm::Type* fieldType = ConvertType(fieldDecl->GetType());
        if (!fieldType)
            return nullptr;
        fields.emplace_back(fieldType);
    }
    llvm::StructType* resultType = llvm::StructType::get(cgm.GetLLVMContext(), fields);
    types.insert(std::make_pair(type.GetType(), resultType));
    return resultType;
}

llvm::FunctionType* VCL::CodeGenTypes::ConvertFunctionType(QualType type) {
    FunctionType* functionType = (FunctionType*)type.GetType();
    llvm::Type* returnType = ConvertType(functionType->GetReturnType());
    llvm::SmallVector<llvm::Type*> paramsType{};
    for (auto paramType : functionType->GetParamsType())
        paramsType.push_back(ConvertType(paramType.GetType()));
    return llvm::FunctionType::get(returnType, paramsType, false);
}

llvm::PointerType* VCL::CodeGenTypes::ConvertReferenceType(QualType type) {
    ReferenceType* referenceType = (ReferenceType*)type.GetType();
    llvm::Type* baseType = ConvertType(referenceType->GetType());
    return llvm::PointerType::get(baseType, 0);
}