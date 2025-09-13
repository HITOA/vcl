#pragma once

#include <VCL/Core/SourceLocation.hpp>
#include <VCL/Core/Identifier.hpp>
#include <VCL/AST/Type.hpp>
#include <VCL/AST/DeclContext.hpp>
#include <VCL/AST/ASTContext.hpp>

#include <string>


namespace VCL {
    
    class Decl {
    public:
        enum DeclClass {
            TranslationUnitDeclClass,
            VarDeclClass,

            TemplateTypeParamDeclClass,
            NonTypeTemplateParamDeclClass,
            IntrinsicTemplateDeclClass
        };
    
    public:
        Decl() = delete;
        Decl(DeclClass declClass) : declClass{ declClass }, range{}, next{ nullptr } {}
        Decl(const Decl& other) = delete;
        Decl(Decl&& other) = delete;
        ~Decl() = default;

        Decl& operator=(const Decl& other) = delete;
        Decl& operator=(Decl&& other) = delete;

        inline DeclClass GetDeclClass() const { return declClass; }
        inline SourceRange GetSourceRange() const { return range; }
        inline void SetSourceRange(SourceRange range) { this->range = range; }

        inline Decl* GetNext() const { return next; }
        inline void SetNext(Decl* next) { this->next = next; }

        inline bool IsNamedDecl() const { return bitfield.isNamedDecl; }
        inline void SetNamedDecl(bool isNamedDecl) { bitfield.isNamedDecl = isNamedDecl; }
        inline bool IsTypeDecl() const { return bitfield.isTypeDecl; }
        inline void SetTypeDecl(bool isTypeDecl) { bitfield.isTypeDecl = isTypeDecl; }
        inline bool IsTemplateDecl() const { return bitfield.isTypeDecl; }
        inline void SetTemplateDecl(bool isTemplateDecl) { bitfield.isTemplateDecl = isTemplateDecl; }

    private:
        DeclClass declClass;
        SourceRange range;
        Decl* next;

        struct DeclBitfield {
            unsigned isNamedDecl : 1 = 0;
            unsigned isTypeDecl : 1 = 0;
            unsigned isTemplateDecl : 1 = 0;
        } bitfield{};
    };

    class TranslationUnitDecl : public Decl, public DeclContext {
    public:
        TranslationUnitDecl() : Decl{ Decl::TranslationUnitDeclClass } {}
        ~TranslationUnitDecl() = default;

        static inline TranslationUnitDecl* Create(ASTContext& context) {
            TranslationUnitDecl* instance = context.AllocateNode<TranslationUnitDecl>();
            return instance;
        }
    };

    class NamedDecl : public Decl {
    public:
        NamedDecl(IdentifierInfo* identifier, DeclClass declClass) : identifier{ identifier }, Decl{ declClass } {
            SetNamedDecl(true);
        }
        ~NamedDecl() = default;

        inline IdentifierInfo* GetIdentifierInfo() const { return identifier; }

    private:
        IdentifierInfo* identifier;
    };

    class TypeDecl : public NamedDecl {
    public:
        TypeDecl(Type* type, IdentifierInfo* identifier, DeclClass declClass) : type{ type }, NamedDecl{ identifier, declClass } {
            SetTypeDecl(true);
        }
        ~TypeDecl() = default;  
        
        inline Type* GetType() const { return type; }
        inline void SetType(Type* type) { this->type = type; }

    private:
        Type* type;
    };

    class VarDecl : public NamedDecl {
    public:
        struct VarAttrBitfield {
            unsigned int hasInAttribute : 1;
            unsigned int hasOutAttribute : 1;
        };
        
    public:
        VarDecl(QualType type, IdentifierInfo* identifier, VarAttrBitfield attr) : 
            type{ type }, varAttrBitfield{ attr }, NamedDecl{ identifier, Decl::VarDeclClass } {}
        ~VarDecl() = default;

        inline QualType GetType() const { return type; }

        inline bool HasInAttribute() const { return varAttrBitfield.hasInAttribute; }
        inline void SetInAttribute(bool b) { varAttrBitfield.hasInAttribute = b; }
        
        inline bool HasOutAttribute() const { return varAttrBitfield.hasOutAttribute; }
        inline void SetOutAttribute(bool b) { varAttrBitfield.hasOutAttribute = b; }

        inline bool HasInoutAttribute() const { return HasInAttribute() && HasOutAttribute(); }

        static inline VarDecl* Create(ASTContext& context, QualType type, IdentifierInfo* identifier, VarAttrBitfield attr, SourceRange range) {
            VarDecl* instance = context.AllocateNode<VarDecl>(type, identifier, attr);
            instance->SetSourceRange(range);
            return instance;
        }

    private:
        QualType type;
        VarAttrBitfield varAttrBitfield;
    };

}