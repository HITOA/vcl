#pragma once

#include <VCL/AST/ASTContext.hpp>
#include <VCL/AST/Template.hpp>
#include <VCL/AST/DeclTemplate.hpp>
#include <VCL/Sema/TreeTransform.hpp>

#include <llvm/ADT/DenseMap.h>


namespace VCL {
    class Sema;

    /** Instantiates templates: a TreeTransform that substitutes the template's arguments for its parameters. */
    class TemplateInstantiator : public TreeTransform {
    public:
        TemplateInstantiator() = delete;
        TemplateInstantiator(Sema& sema) : TreeTransform{ sema }, substitutionTable{} {}
        TemplateInstantiator(const TemplateInstantiator& other) = delete;
        TemplateInstantiator(TemplateInstantiator&& other) = delete;
        ~TemplateInstantiator() override = default;
        
        TemplateInstantiator& operator=(const TemplateInstantiator& other) = delete;
        TemplateInstantiator& operator=(TemplateInstantiator&& other) = delete;
        
        bool MakeTypeComplete(Type* type);
        bool InstantiateTemplateSpecializationType(TemplateSpecializationType* type);
        bool AddTemplateArgumentListAndDecl(TemplateArgumentList* args, TemplateDecl* decl);
        bool CheckTemplateArgumentsParametersMatch(TemplateArgumentList* args, TemplateParameterList* params);
        bool EvaluateTemplateArgumentsExpr(TemplateArgumentList* args);

        Type* InstantiateTemplatedIntrinsicTypeDecl(TemplateDecl* decl);
        Type* InstantiateTemplatedRecordDecl(TemplateDecl* decl);
        Type* InstantiateTemplatedTypeAliasDecl(TemplateDecl* decl);
        FunctionDecl* InstantiateTemplatedFunctionDecl(TemplateDecl* decl);

        void AddSubstitution(NamedDecl* param, TemplateArgument* arg);
        TemplateArgument* Lookup(NamedDecl* param);

        // Substitution of the template parameters (the rest of the walk is TreeTransform's)

        QualType TransformTemplateTypeParamType(QualType type) override;
        QualType TransformTemplateSpecializationType(QualType type) override;
        QualType TransformTypeAliasType(QualType type) override;
        Expr* TransformDeclRefExpr(DeclRefExpr* expr) override;

    private:
        llvm::DenseMap<NamedDecl*, TemplateArgument*> substitutionTable;
    };

}