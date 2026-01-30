#include <VCL/AST/TypePrinter.hpp>

#include <VCL/AST/Decl.hpp>
#include <VCL/AST/DeclTemplate.hpp>
#include <VCL/AST/Template.hpp>


std::string VCL::TypePrinter::Print(QualType type) {
    if (type.GetAsOpaquePtr() == 0)
        return "nullptr";
    std::string qualifiers = PrintQualifier(type);
    switch (type.GetType()->GetTypeClass()) {
        case Type::BuiltinTypeClass:
            return qualifiers + PrintBuiltinType((BuiltinType*)type.GetType());
        case Type::ReferenceTypeClass:
            return qualifiers + PrintReferenceType((ReferenceType*)type.GetType());
        case Type::VectorTypeClass:
            return qualifiers + PrintVectorType((VectorType*)type.GetType());
        case Type::LanesTypeClass:
            return qualifiers + PrintLanesType((LanesType*)type.GetType());
        case Type::ArrayTypeClass:
            return qualifiers + PrintArrayType((ArrayType*)type.GetType());
        case Type::SpanTypeClass:
            return qualifiers + PrintSpanType((SpanType*)type.GetType());
        case Type::RecordTypeClass:
            return qualifiers + PrintRecordType((RecordType*)type.GetType());
        case Type::FunctionTypeClass:
            return qualifiers + PrintFunctionType((FunctionType*)type.GetType());
        case Type::TemplateTypeParamTypeClass:
            return qualifiers + PrintTemplateTypeParamType((TemplateTypeParamType*)type.GetType());
        case Type::TemplateSpecializationTypeClass:
            return qualifiers + PrintTemplateSpecializationType((TemplateSpecializationType*)type.GetType());
        default:
            return "invalid";
    }
}

std::string VCL::TypePrinter::PrintQualifier(QualType type) {
    std::string qualifiers = "";
    if (type.HasQualifier(Qualifier::Const))
        qualifiers += "const ";
    return qualifiers;
}

std::string VCL::TypePrinter::PrintBuiltinType(BuiltinType* type) {
    switch (type->GetKind()) {
        case BuiltinType::Void: return "void";
        case BuiltinType::Float32: return "float32";
        case BuiltinType::Float64: return "float64";
        case BuiltinType::Int8: return "int8";
        case BuiltinType::Int16: return "int16";
        case BuiltinType::Int32: return "int32";
        case BuiltinType::Int64: return "int64";
        case BuiltinType::Bool: return "bool";
        case BuiltinType::UInt8: return "uint8";
        case BuiltinType::UInt16: return "uint16";
        case BuiltinType::UInt32: return "uint32";
        case BuiltinType::UInt64: return "uint64";
        default:
            return "invalid";
    }
}

std::string VCL::TypePrinter::PrintReferenceType(ReferenceType* type) {
    return Print(type->GetType());
}

std::string VCL::TypePrinter::PrintVectorType(VectorType* type) {
    return "Vec<" + Print(type->GetElementType()) + ">";
}

std::string VCL::TypePrinter::PrintLanesType(LanesType* type) {
    return "Lanes<" + Print(type->GetElementType()) + ">";
}

std::string VCL::TypePrinter::PrintArrayType(ArrayType* type) {
    return "Array<" + Print(type->GetElementType()) + ", " + std::to_string(type->GetElementCount()) + ">";
}

std::string VCL::TypePrinter::PrintSpanType(SpanType* type) {
    return "Span<" + Print(type->GetElementType()) + ">";
}

std::string VCL::TypePrinter::PrintRecordType(RecordType* type) {
    return type->GetRecordDecl()->GetIdentifierInfo()->GetName().str();
}

std::string VCL::TypePrinter::PrintFunctionType(FunctionType* type) {
    std::string result = Print(type->GetReturnType()) + "(";

    llvm::ArrayRef<QualType> args = type->GetParamsType();
    for (uint32_t i = 0; i < args.size(); ++i) {
        result += Print(args[i]);
        if (i + 1 != args.size())
            result += ",";
    }

    return result + ")";
}

std::string VCL::TypePrinter::PrintTemplateTypeParamType(TemplateTypeParamType* type) {
    return type->GetTemplateTypeParamDecl()->GetIdentifierInfo()->GetName().str();
}

std::string VCL::TypePrinter::PrintTemplateSpecializationType(TemplateSpecializationType* type) {
    std::string result = type->GetTemplateDecl()->GetTemplatedNamedDecl()->GetIdentifierInfo()->GetName().str() += "<";
    for (size_t i = 0; i < type->GetTemplateArgumentList()->GetCount(); ++i) {
        TemplateArgument arg = type->GetTemplateArgumentList()->GetData()[i];
        std::string_view args{ arg.GetSourceRange().start.GetPtr(), (size_t)(arg.GetSourceRange().end.GetPtr() - arg.GetSourceRange().start.GetPtr()) };
        switch (arg.GetKind()) {
            case TemplateArgument::Kind::Type: {
                result += Print(arg.GetType());
                break;
            }
            case TemplateArgument::Kind::Integral: {
                switch (arg.GetIntegral().GetKind()) {
                    case BuiltinType::Float32: {
                        result += std::to_string(arg.GetIntegral().Get<float>());
                        break;
                    }
                    case BuiltinType::Float64: {
                        result += std::to_string(arg.GetIntegral().Get<double>());
                        break;
                    }
                    case BuiltinType::Int8:
                    case BuiltinType::Int16:
                    case BuiltinType::Int32:
                    case BuiltinType::Int64: {
                        result += std::to_string(arg.GetIntegral().Get<int64_t>());
                        break;
                    }
                    case BuiltinType::UInt8:
                    case BuiltinType::UInt16:
                    case BuiltinType::UInt32:
                    case BuiltinType::UInt64: {
                        result += std::to_string(arg.GetIntegral().Get<uint64_t>());
                        break;
                    }
                }
                break;
            }
            default: { 
                result += args;
                break;
            }
        }
        if (i + 1 < type->GetTemplateArgumentList()->GetCount())
            result += ", ";
    }
    result += ">";
    return result;
}