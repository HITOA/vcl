#pragma once

#include <VCL/AST/Decl.hpp>
#include <VCL/AST/DeclTemplate.hpp>


namespace VCL {

    template<typename T>
    class DeclVisitor {
    public:
        inline T VisitDecl(Decl* decl) {
            switch (decl->GetDeclClass()) {
                case Decl::TranslationUnitDeclClass: return VisitTranslationUnitDecl((TranslationUnitDecl*)decl);
                case Decl::FieldDeclClass: return VisitFieldDecl((FieldDecl*)decl);
                case Decl::RecordDeclClass: return VisitRecordDecl((RecordDecl*)decl);
                case Decl::VarDeclClass: return VisitVarDecl((VarDecl*)decl);
                case Decl::ParamDeclClass: return VisitParamDecl((ParamDecl*)decl);
                case Decl::FunctionDeclClass: return VisitFunctionDecl((FunctionDecl*)decl);
                case Decl::IntrinsicTypeDeclClass: return VisitIntrinsicTypeDecl((IntrinsicTypeDecl*)decl);
                case Decl::DirectiveDeclClass: return VisitDirectiveDecl((DirectiveDecl*)decl);
                case Decl::TemplateTypeParamDeclClass: return VisitTemplateTypeParamDecl((TemplateTypeParamDecl*)decl);
                case Decl::NonTypeTemplateParamDeclClass: return VisitNonTypeTemplateParamDecl((NonTypeTemplateParamDecl*)decl);
                case Decl::TemplateDeclClass: return VisitTemplateDecl((TemplateDecl*)decl);
                case Decl::TemplateSpecializationDeclClass: return VisitTemplateSpecializationDecl((TemplateSpecializationDecl*)decl);
                default: return T{};
            }
        }

    protected:
        virtual T VisitTranslationUnitDecl(TranslationUnitDecl* decl) = 0;
        virtual T VisitFieldDecl(FieldDecl* decl) = 0;
        virtual T VisitRecordDecl(RecordDecl* decl) = 0;
        virtual T VisitVarDecl(VarDecl* decl) = 0;
        virtual T VisitParamDecl(ParamDecl* decl) = 0;
        virtual T VisitFunctionDecl(FunctionDecl* decl) = 0;
        virtual T VisitIntrinsicTypeDecl(IntrinsicTypeDecl* decl) = 0;
        virtual T VisitDirectiveDecl(DirectiveDecl* decl) = 0;
        virtual T VisitTemplateTypeParamDecl(TemplateTypeParamDecl* decl) = 0;
        virtual T VisitNonTypeTemplateParamDecl(NonTypeTemplateParamDecl* decl) = 0;
        virtual T VisitTemplateDecl(TemplateDecl* decl) = 0;
        virtual T VisitTemplateSpecializationDecl(TemplateSpecializationDecl* decl) = 0;
    };

}