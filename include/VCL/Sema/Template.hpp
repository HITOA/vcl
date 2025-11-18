#pragma once

#include <VCL/AST/ASTContext.hpp>
#include <VCL/AST/Template.hpp>
#include <VCL/AST/DeclTemplate.hpp>

#include <llvm/ADT/DenseMap.h>


namespace VCL {
    class Sema;

    class TemplateInstantiator {
    public:
        TemplateInstantiator() = delete;
        TemplateInstantiator(Sema& sema) : sema{ sema }, substitutionTable{} {}
        TemplateInstantiator(const TemplateInstantiator& other) = delete;
        TemplateInstantiator(TemplateInstantiator&& other) = delete;
        ~TemplateInstantiator() = default;
        
        TemplateInstantiator& operator=(const TemplateInstantiator& other) = delete;
        TemplateInstantiator& operator=(TemplateInstantiator&& other) = delete;
        
        bool MakeTypeComplete(Type* type);
        bool InstantiateTemplateSpecializationType(TemplateSpecializationType* type);
        bool AddTemplateArgumentListAndDecl(TemplateArgumentList* args, TemplateDecl* decl);
        bool CheckTemplateArgumentsParametersMatch(TemplateArgumentList* args, TemplateParameterList* params);
        bool EvaluateTemplateArgumentsExpr(TemplateArgumentList* args);

        Type* InstantiateIntrinsicTemplateDecl(IntrinsicTemplateDecl* decl);
        RecordType* InstantiateTemplateRecordDecl(TemplateRecordDecl* decl);

        void AddSubstitution(NamedDecl* param, TemplateArgument* arg);
        TemplateArgument* Lookup(NamedDecl* param);

        QualType TransformType(QualType type);
        Decl* TransformDecl(Decl* decl);
        Expr* TransformExpr(Expr* expr);

        QualType TransformTemplateTypeParamType(QualType type);
        QualType TransformTemplateSpecializationType(QualType type);

        Decl* TransformFieldDecl(FieldDecl* decl);

        Expr* TransformDeclRefExpr(DeclRefExpr* expr);
        Expr* TransformCastExpr(CastExpr* expr);
        Expr* TransformBinaryExpr(BinaryExpr* expr);

    private:
        Sema& sema;
        llvm::DenseMap<NamedDecl*, TemplateArgument*> substitutionTable;
    };

}