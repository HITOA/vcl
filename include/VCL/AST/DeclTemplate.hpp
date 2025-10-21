#pragma once

#include <VCL/AST/Decl.hpp>

#include <llvm/Support/TrailingObjects.h>
#include <llvm/ADT/PointerUnion.h>


namespace VCL {

    class TemplateTypeParamDecl : public TypeDecl {
    public:
        TemplateTypeParamDecl(IdentifierInfo* identifier) : TypeDecl{ nullptr, identifier, Decl::TemplateTypeParamDeclClass } {}
        ~TemplateTypeParamDecl() = default;

        static inline TemplateTypeParamDecl* Create(ASTContext& context, IdentifierInfo* identifier, SourceRange range) {
            TemplateTypeParamDecl* decl = context.AllocateNode<TemplateTypeParamDecl>(identifier);
            TemplateTypeParamType* type = context.GetTypeCache().GetOrCreateTemplateTypeParamType(decl);
            decl->SetType(type);
            decl->SetSourceRange(range);
            return decl;
        }
    };

    class NonTypeTemplateParamDecl : public ValueDecl {
    public:
        NonTypeTemplateParamDecl(BuiltinType* type, IdentifierInfo* identifier) 
            : ValueDecl{ type, identifier, Decl::NonTypeTemplateParamDeclClass } {}
        ~NonTypeTemplateParamDecl() = default;
        
        inline BuiltinType* GetType() const { return (BuiltinType*)GetValueType().GetType(); }

        static inline NonTypeTemplateParamDecl* Create(ASTContext& context, BuiltinType* type, IdentifierInfo* identifier, SourceRange range) {
            NonTypeTemplateParamDecl* decl = context.AllocateNode<NonTypeTemplateParamDecl>(type, identifier);
            decl->SetSourceRange(range);
            return decl;
        }
    };

    using TemplateParameter = llvm::PointerUnion<TemplateTypeParamDecl*, NonTypeTemplateParamDecl*>;

    class TemplateParameterList final : private llvm::TrailingObjects<TemplateParameterList, NamedDecl*> {
        friend TrailingObjects;
    
    public:
        TemplateParameterList() = delete;
        TemplateParameterList(llvm::ArrayRef<NamedDecl*> params, SourceRange range) : paramCount{ params.size() }, range{ range } {
            std::uninitialized_copy(params.begin(), params.end(), getTrailingObjects<NamedDecl*>());
        }
        TemplateParameterList(const TemplateParameterList& other) = delete;
        TemplateParameterList(TemplateParameterList&& other) = delete;
        ~TemplateParameterList() = default;

        TemplateParameterList& operator=(const TemplateParameterList& other) = delete;
        TemplateParameterList& operator=(TemplateParameterList&& other) = delete;

        inline llvm::ArrayRef<NamedDecl*> GetParams() { return { getTrailingObjects<NamedDecl*>(), paramCount }; }
        inline SourceRange GetSourceRange() const { return range; }

        static inline TemplateParameterList* Create(ASTContext& context, llvm::ArrayRef<NamedDecl*> params, SourceRange range) {
            size_t size = totalSizeToAlloc<NamedDecl*>(params.size());
            void* ptr = context.Allocate(sizeof(TemplateParameterList) + size);
            return new(ptr) TemplateParameterList{ params, range };
        }

    private:
        size_t paramCount;
        SourceRange range;
    };

    class TemplateDecl : public NamedDecl {
    public:
        TemplateDecl(TemplateParameterList* params, IdentifierInfo* identifier, DeclClass declClass) 
            : params{ params }, NamedDecl{ identifier, declClass } {
            SetTemplateDecl(true);
        }
        ~TemplateDecl() = default;

        inline TemplateParameterList* GetTemplateParametersList() { return params; }

    private:
        TemplateParameterList* params;
    };

    /**
     * This class is for all intrinsic templated type. (ex: Vec, Array, Span)
     */
    class IntrinsicTemplateDecl : public TemplateDecl {
    public:
        IntrinsicTemplateDecl(TemplateParameterList* params, IdentifierInfo* identifier) 
            : TemplateDecl{ params, identifier, DeclClass::IntrinsicTemplateDeclClass } {}
        ~IntrinsicTemplateDecl() = default;

        static inline IntrinsicTemplateDecl* Create(ASTContext& context, TemplateParameterList* params, IdentifierInfo* identifier) {
            return context.AllocateNode<IntrinsicTemplateDecl>(params, identifier);
        }
    };

    class TemplateRecordDecl : public TemplateDecl, public DeclContext {
    public:
        TemplateRecordDecl(TemplateParameterList* params, IdentifierInfo* identifier) 
                : TemplateDecl{ params, identifier, DeclClass::TemplateRecordDeclClass }, DeclContext{ DeclContext::TemplateRecordDeclContextClass } {
            SetDeclContext(true);
        }
        ~TemplateRecordDecl() = default;

        static inline TemplateRecordDecl* Create(ASTContext& context, IdentifierInfo* identifier, TemplateParameterList* params, SourceRange range) {
            TemplateRecordDecl* decl = context.AllocateNode<TemplateRecordDecl>(params, identifier);
            for (auto param : params->GetParams())
                decl->InsertBack(param);
            decl->SetSourceRange(range);
            return decl;
        }
    };

}