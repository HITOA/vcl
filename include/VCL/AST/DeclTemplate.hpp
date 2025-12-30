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
            std::uninitialized_copy(params.begin(), params.end(), getTrailingObjects());
        }
        TemplateParameterList(const TemplateParameterList& other) = delete;
        TemplateParameterList(TemplateParameterList&& other) = delete;
        ~TemplateParameterList() = default;

        TemplateParameterList& operator=(const TemplateParameterList& other) = delete;
        TemplateParameterList& operator=(TemplateParameterList&& other) = delete;

        inline llvm::ArrayRef<NamedDecl*> GetParams() { return { getTrailingObjects(), paramCount }; }
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

    class TemplateSpecializationDecl : public Decl {
    public:
        TemplateSpecializationDecl(TemplateArgumentList* arguments, NamedDecl* decl)
                : arguments{ arguments }, decl{ decl }, Decl{ Decl::TemplateSpecializationDeclClass } {}
        ~TemplateSpecializationDecl() = default;

        inline TemplateArgumentList* GetTemplateArgumentList() { return arguments; }
        inline NamedDecl* GetNamedDecl() { return decl; }

        static inline TemplateSpecializationDecl* Create(ASTContext& context, TemplateArgumentList* arguments, NamedDecl* decl) {
            TemplateSpecializationDecl* instance = context.AllocateNode<TemplateSpecializationDecl>(arguments, decl);
            return instance;
        }

    private:
        TemplateArgumentList* arguments;
        NamedDecl* decl;
    };

    class TemplateDecl : public Decl, public DeclContext {
    public:
        TemplateDecl(TemplateParameterList* params)
                : params{ params }, decl{ nullptr }, Decl{ Decl::TemplateDeclClass }, DeclContext{ DeclContext::TemplateDeclContextClass } {
            SetTemplateDecl(true);
        }
        ~TemplateDecl() = default;

        inline TemplateParameterList* GetTemplateParametersList() { return params; }
        inline void SetTemplatedNamedDecl(NamedDecl* decl) { this->decl = decl; }
        inline NamedDecl* GetTemplatedNamedDecl() { return decl; }

        static inline TemplateDecl* Create(ASTContext& context, TemplateParameterList* params, SourceRange range) {
            TemplateDecl* instance = context.AllocateNode<TemplateDecl>(params);
            for (auto param : params->GetParams())
                instance->InsertBack(param);
            instance->SetSourceRange(range);
            return instance;
        }

    private:
        TemplateParameterList* params;
        NamedDecl* decl;
    };

}