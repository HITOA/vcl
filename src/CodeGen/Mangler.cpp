#include <VCL/CodeGen/Mangler.hpp>

#include <VCL/Core/Hasher.hpp>

#include <sstream>
#include <iomanip>


std::string VCL::Mangler::MangleFunctionDecl(ASTContext& context, FunctionDecl* decl) {
    Hasher prefixHash{};
    Hasher suffixHash{};

    prefixHash.Hash((uint64_t)(&context));
    suffixHash.Hash(HashType(decl->GetType()));

    std::stringstream r{};
    r << std::setw(16) << std::setfill('0') << std::hex << prefixHash.Get()
        << "_" << decl->GetIdentifierInfo()->GetName().str() << "_"
        << std::setw(16) << std::setfill('0') << std::hex << suffixHash.Get();
    return r.str();
}

std::string VCL::Mangler::MangleVarDecl(ASTContext& context, VarDecl* decl) {
    Hasher prefixHash{};
    Hasher suffixHash{};

    prefixHash.Hash((uint64_t)(&context));
    suffixHash.Hash(HashType(decl->GetValueType()));

    std::stringstream r{};
    r << std::setw(16) << std::setfill('0') << std::hex << prefixHash.Get()
        << "_" << decl->GetIdentifierInfo()->GetName().str() << "_"
        << std::setw(16) << std::setfill('0') << std::hex << suffixHash.Get();
    return r.str();
}

uint64_t VCL::Mangler::HashType(QualType type) {
    llvm::FoldingSetNodeID typeNodeId{};
    typeNodeId.AddInteger(type.GetQualifiers());
    switch (type.GetType()->GetTypeClass()) {
        case Type::BuiltinTypeClass: {
            ((BuiltinType*)type.GetType())->Profile(typeNodeId);
            break;
        }
        case Type::ReferenceTypeClass: {
            ((ReferenceType*)type.GetType())->Profile(typeNodeId);
            break;
        }
        case Type::VectorTypeClass: {
            ((VectorType*)type.GetType())->Profile(typeNodeId);
            break;
        }
        case Type::ArrayTypeClass: {
            ((ArrayType*)type.GetType())->Profile(typeNodeId);
            break;
        }
        case Type::SpanTypeClass: {
            ((SpanType*)type.GetType())->Profile(typeNodeId);
            break;
        }
        case Type::RecordTypeClass: {
            ((RecordType*)type.GetType())->Profile(typeNodeId);
            break;
        }
        case Type::FunctionTypeClass: {
            ((FunctionType*)type.GetType())->Profile(typeNodeId);
            break;
        }
        case Type::TemplateTypeParamTypeClass: {
            ((TemplateTypeParamType*)type.GetType())->Profile(typeNodeId);
            break;
        }
        case Type::TemplateSpecializationTypeClass: {
            ((TemplateSpecializationType*)type.GetType())->Profile(typeNodeId);
            break;
        }
        case Type::DependentTypeClass: {
            ((DependentType*)type.GetType())->Profile(typeNodeId);
            break;
        }
        default: typeNodeId.AddPointer(type.GetType());
    }
    return (uint64_t)typeNodeId.ComputeHash();
}