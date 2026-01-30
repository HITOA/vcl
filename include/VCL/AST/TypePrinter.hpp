#pragma once

#include <VCL/AST/Type.hpp>

#include <string>


namespace VCL {

    class TypePrinter {
    public:
        static std::string Print(QualType type);

    private:
        static std::string PrintQualifier(QualType type);

        static std::string PrintBuiltinType(BuiltinType* type);
        static std::string PrintReferenceType(ReferenceType* type);
        static std::string PrintVectorType(VectorType* type);
        static std::string PrintLanesType(LanesType* type);
        static std::string PrintArrayType(ArrayType* type);
        static std::string PrintSpanType(SpanType* type);
        static std::string PrintRecordType(RecordType* type);
        static std::string PrintFunctionType(FunctionType* type);
        static std::string PrintTemplateTypeParamType(TemplateTypeParamType* type);
        static std::string PrintTemplateSpecializationType(TemplateSpecializationType* type);
    };

}