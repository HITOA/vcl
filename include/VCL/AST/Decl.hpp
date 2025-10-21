#pragma once

#include <VCL/Core/SourceLocation.hpp>
#include <VCL/Core/Identifier.hpp>
#include <VCL/AST/Type.hpp>
#include <VCL/AST/DeclContext.hpp>
#include <VCL/AST/ASTContext.hpp>

#include <string>


namespace VCL {
    class Stmt;
    class Expr;

    class Decl {
    public:
        enum DeclClass {
            TranslationUnitDeclClass,
            FieldDeclClass,
            RecordDeclClass,
            VarDeclClass,
            ParamDeclClass,
            TransientFunctionDeclClass,
            FunctionDeclClass,

            TemplateTypeParamDeclClass,
            NonTypeTemplateParamDeclClass,
            IntrinsicTemplateDeclClass,
            TemplateRecordDeclClass
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
        inline bool IsValueDecl() const { return bitfield.isValueDecl; }
        inline void SetValueDecl(bool isValueDecl) { bitfield.isValueDecl = isValueDecl; }
        inline bool IsTemplateDecl() const { return bitfield.isTemplateDecl; }
        inline void SetTemplateDecl(bool isTemplateDecl) { bitfield.isTemplateDecl = isTemplateDecl; }
        inline bool IsDeclContext() const { return bitfield.isDeclContext; }
        inline void SetDeclContext(bool isDeclContext) { bitfield.isDeclContext = isDeclContext; }

    private:
        DeclClass declClass;
        SourceRange range;
        Decl* next;

        struct DeclBitfield {
            unsigned isNamedDecl : 1 = 0;
            unsigned isTypeDecl : 1 = 0;
            unsigned isValueDecl : 1 = 0;
            unsigned isTemplateDecl : 1 = 0;
            unsigned isDeclContext : 1 = 0;
        } bitfield{};
    };

    class TranslationUnitDecl : public Decl, public DeclContext {
    public:
        TranslationUnitDecl() : Decl{ Decl::TranslationUnitDeclClass }, DeclContext{ DeclContext::TranslationUnitDeclContextClass } {
            SetDeclContext(true);
        }
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

    class ValueDecl : public NamedDecl {
    public:
        ValueDecl(QualType valueType, IdentifierInfo* identifier, DeclClass declClass) : valueType{ valueType }, NamedDecl{ identifier, declClass } {
            SetValueDecl(true);
        }
        ~ValueDecl() = default;

        inline QualType GetValueType() const { return valueType; }

    private:
        QualType valueType;
    };

    class FieldDecl : public NamedDecl {
    public:
        FieldDecl(IdentifierInfo* identifier, QualType type) : ofType{ type }, NamedDecl{ identifier, DeclClass::FieldDeclClass } {}
        ~FieldDecl() = default;

        inline QualType GetType() const { return ofType; }

        static inline FieldDecl* Create(ASTContext& context, IdentifierInfo* identifier, QualType type, SourceRange range) {
            FieldDecl* decl = context.AllocateNode<FieldDecl>(identifier, type);
            decl->SetSourceRange(range);
            return decl;
        }

    private:
        QualType ofType;
    };

    class RecordDecl : public TypeDecl, public DeclContext {
    public:
        RecordDecl(IdentifierInfo* identifier) : TypeDecl{ nullptr, identifier, Decl::RecordDeclClass }, 
                DeclContext{ DeclContext::RecordDeclContextClass } {
            SetDeclContext(true);
        }
        ~RecordDecl() = default;

        static inline RecordDecl* Create(ASTContext& context, IdentifierInfo* identifier, SourceRange range) {
            RecordDecl* decl = context.AllocateNode<RecordDecl>(identifier);
            RecordType* type = context.GetTypeCache().GetOrCreateRecordType(decl);
            decl->SetType(type);
            decl->SetSourceRange(range);
            return decl;
        }
    };

    class VarDecl : public ValueDecl {
    public:
        struct VarAttrBitfield {
            unsigned int hasInAttribute : 1;
            unsigned int hasOutAttribute : 1;
        };
        
    public:
        VarDecl(QualType type, IdentifierInfo* identifier, VarAttrBitfield attr) : 
            attrBitfield{ attr }, initializer{ nullptr }, ValueDecl{ type, identifier, Decl::VarDeclClass } {}
        ~VarDecl() = default;

        inline bool HasInAttribute() const { return attrBitfield.hasInAttribute; }
        inline void SetInAttribute(bool b) { attrBitfield.hasInAttribute = b; }
        
        inline bool HasOutAttribute() const { return attrBitfield.hasOutAttribute; }
        inline void SetOutAttribute(bool b) { attrBitfield.hasOutAttribute = b; }

        inline bool HasInoutAttribute() const { return HasInAttribute() && HasOutAttribute(); }

        inline Expr* GetInitializer() const { return initializer; }
        inline void SetInitializer(Expr* initializer) { this->initializer = initializer; }

        static inline VarDecl* Create(ASTContext& context, QualType type, IdentifierInfo* identifier, VarAttrBitfield attr, SourceRange range) {
            VarDecl* instance = context.AllocateNode<VarDecl>(type, identifier, attr);
            instance->SetSourceRange(range);
            return instance;
        }

    private:
        VarAttrBitfield attrBitfield;
        Expr* initializer;
    };

    class ParamDecl : public ValueDecl {
    public:
        ParamDecl(QualType type, IdentifierInfo* identifier) :  ValueDecl{ type, identifier, Decl::ParamDeclClass } {}
        ~ParamDecl() = default;

        static inline ParamDecl* Create(ASTContext& context, QualType type, IdentifierInfo* identifier, SourceRange range) {
            ParamDecl* instance = context.AllocateNode<ParamDecl>(type, identifier);
            instance->SetSourceRange(range);
            return instance;
        }
    };

    class TransientFunctionDecl : public NamedDecl, public DeclContext {
    public:
        TransientFunctionDecl(QualType returnType, IdentifierInfo* identifier) : returnType{ returnType }, 
                NamedDecl{ identifier, Decl::TransientFunctionDeclClass }, DeclContext{ DeclContext::TransientFunctionDeclContextClass } {
            SetDeclContext(true);
        }
        ~TransientFunctionDecl() = default;

        inline QualType GetReturnType() const { return returnType; }

        static inline TransientFunctionDecl* Create(ASTContext& astContext, QualType returnType, IdentifierInfo* identifier, SourceRange range) {
            TransientFunctionDecl* instance = astContext.AllocateNode<TransientFunctionDecl>(returnType, identifier);
            instance->SetSourceRange(range);
            return instance;
        }
        
    private:
        QualType returnType;
    };

    class FunctionDecl : public NamedDecl, public DeclContext {
    public:
        FunctionDecl(FunctionType* type, IdentifierInfo* identifier) : type{ type },
            NamedDecl{ identifier, Decl::FunctionDeclClass }, DeclContext{ DeclContext::FunctionDeclContextClass } {
            SetDeclContext(true);
        }
        ~FunctionDecl() = default;

        inline FunctionType* GetType() const { return type; }

        inline void SetBody(Stmt* body) { this->body = body; }
        inline Stmt* GetBody() const { return body; }

        static inline FunctionDecl* Create(ASTContext& astContext, TransientFunctionDecl* transient, SourceRange range) {
            QualType returnType = transient->GetReturnType();
            llvm::SmallVector<QualType> paramsType{};
            for (auto it = transient->Begin(); it != transient->End(); ++it) {
                if (it->GetDeclClass() == Decl::ParamDeclClass) {
                    ParamDecl* paramDecl = (ParamDecl*)it.Get();
                    paramsType.push_back(paramDecl->GetValueType());
                }
            }
            FunctionType* type = astContext.GetTypeCache().GetOrCreateFunctionType(returnType, paramsType);
            FunctionDecl* decl = astContext.AllocateNode<FunctionDecl>(type, transient->GetIdentifierInfo());
            for (auto it = transient->Begin(); it != transient->End(); ++it)
                decl->InsertBack(it.Get());
            decl->SetSourceRange(range);
            return decl;
        }

    private:
        FunctionType* type;
        Stmt* body;
    };

}