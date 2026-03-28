#pragma once

#include <VCL/AST/Decl.hpp>
#include <VCL/AST/DeclTemplate.hpp>
#include <VCL/AST/Template.hpp>

#include <llvm/ADT/DenseMap.h>


namespace VCL {
    class Sema;

    class TemplateArgumentDeducer {
    public:
        TemplateArgumentDeducer(Sema& sema, TemplateParameterList* parameters);
        TemplateArgumentDeducer(const TemplateArgumentDeducer& other) = delete;
        TemplateArgumentDeducer(TemplateArgumentDeducer&& other) = delete;
        ~TemplateArgumentDeducer() = default;

        TemplateArgumentDeducer& operator=(const TemplateArgumentDeducer& other) = delete;
        TemplateArgumentDeducer& operator=(TemplateArgumentDeducer&& other) = delete;
        
        bool FillSubstitutionMapWithTemplateArgumentList(TemplateArgumentList* args);
        bool IsDeductionComplete();
        bool DeduceForType(Type* baseType, Type* substitutedType);
        TemplateArgumentList* BuildTemplateArgumentList(bool makeCanonical);

    private:
        Sema& sema;

        TemplateParameterList* parameters;
        llvm::DenseMap<NamedDecl*, TemplateArgument> substitutionMap;
    };

}