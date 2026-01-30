#pragma once

#include <VCL/Core/Attribute.hpp>
#include <VCL/Core/SourceLocation.hpp>
#include <VCL/Core/Identifier.hpp>
#include <VCL/AST/Type.hpp>
#include <VCL/AST/DeclContext.hpp>
#include <VCL/AST/ASTContext.hpp>
#include <VCL/AST/AttributeInstance.hpp>

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
            FunctionDeclClass,
            IntrinsicTypeDeclClass,
            DirectiveDeclClass,

            TemplateTypeParamDeclClass,
            NonTypeTemplateParamDeclClass,
            TemplateDeclClass,
            TemplateSpecializationDeclClass
        };


        struct VarAttrBitfield {
            unsigned int hasInAttribute : 1;
            unsigned int hasOutAttribute : 1;
        };
    
    public:
        Decl() = delete;
        Decl(DeclClass declClass) : declClass{ declClass }, range{}, next{ nullptr }, attribute{ nullptr } {}
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

        inline AttributeInstance* GetAttribute() const { return attribute; }
        inline void PushAttribute(AttributeInstance* attribute) {
            if (!this->attribute) {
                this->attribute = attribute;
                return;
            }
            AttributeInstance* currentBack = this->attribute;
            while (currentBack->GetNextAttribute() != nullptr)
                currentBack = currentBack->GetNextAttribute();
            currentBack->SetNextAttribute(attribute);
        }
        inline AttributeInstance* HasAttribute(AttributeDefinition* definition) {
            if (!attribute)
                return nullptr;
            AttributeInstance* currentAttribute = attribute;
            while (currentAttribute != nullptr) {
                if (currentAttribute->GetDefinition() == definition)
                    return currentAttribute;
                currentAttribute = currentAttribute->GetNextAttribute();
            }
            return nullptr;
        }

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
        AttributeInstance* attribute;

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

        inline void SetValueType(QualType valueType) { this->valueType = valueType; }
        inline QualType GetValueType() const { return valueType; }

    private:
        QualType valueType;
    };

    class IntrinsicTypeDecl : public NamedDecl {
    public:
        IntrinsicTypeDecl(IdentifierInfo* identifier) : NamedDecl{ identifier, Decl::IntrinsicTypeDeclClass } {}
        ~IntrinsicTypeDecl() = default;

        inline static IntrinsicTypeDecl* Create(ASTContext& astContext, IdentifierInfo* identifier) {
            return astContext.AllocateNode<IntrinsicTypeDecl>(identifier);
        }
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
        VarDecl(QualType type, IdentifierInfo* identifier, VarAttrBitfield attr) : 
            attrBitfield{ attr }, initializer{ nullptr }, ValueDecl{ type, identifier, Decl::VarDeclClass } {}
        ~VarDecl() = default;

        inline bool HasInAttribute() const { return attrBitfield.hasInAttribute; }
        inline void SetInAttribute(bool b) { attrBitfield.hasInAttribute = b; }
        
        inline bool HasOutAttribute() const { return attrBitfield.hasOutAttribute; }
        inline void SetOutAttribute(bool b) { attrBitfield.hasOutAttribute = b; }

        inline bool HasInoutAttribute() const { return HasInAttribute() && HasOutAttribute(); }

        inline VarAttrBitfield GetVarAttrBitfield() const { return attrBitfield; }

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
        ParamDecl(QualType type, IdentifierInfo* identifier, VarAttrBitfield attr) 
                : ValueDecl{ type, identifier, Decl::ParamDeclClass }, attrBitfield{ attr } {}
        ~ParamDecl() = default;

        inline bool HasInAttribute() const { return attrBitfield.hasInAttribute; }
        inline void SetInAttribute(bool b) { attrBitfield.hasInAttribute = b; }
        
        inline bool HasOutAttribute() const { return attrBitfield.hasOutAttribute; }
        inline void SetOutAttribute(bool b) { attrBitfield.hasOutAttribute = b; }

        inline bool HasInoutAttribute() const { return HasInAttribute() && HasOutAttribute(); }

        inline VarAttrBitfield GetVarAttrBitfield() const { return attrBitfield; }

        static inline ParamDecl* Create(ASTContext& context, QualType type, IdentifierInfo* identifier, VarAttrBitfield attr, SourceRange range) {
            ParamDecl* instance = context.AllocateNode<ParamDecl>(type, identifier, attr);
            instance->SetSourceRange(range);
            return instance;
        }

    private:
        VarAttrBitfield attrBitfield;
    };

    class FunctionDecl : public NamedDecl, public DeclContext {
    public: 
        enum FunctionFlags : uint32_t {
            None = 0,
            IsTemplateSpecialization = 1,
            IsIntrinsic = 2
        };

        enum IntrinsicID : uint32_t {
            NotIntrinsic = 0,
            // Unary Math Intinsic
            Sin, Cos, Tan, Sinh, Cosh, Tanh,
            ASin, ACos, ATan, ATan2,
            Sqrt, Log, Log2, Log10,
            Exp, Exp2,
            Floor, Ceil, Round, Abs,
            // Unary Vec Intrinsic
            Unpack, Pack, Reverse,
            // Unary Array Intrinsic
            Length,
            // Binary Math Intrinsic
            Pow, Min, Max,
            FMod,
            // Ternary Math Intrinsic
            Fma,
            // Other Intrinsic
            Select
        };

    public:
        FunctionDecl(FunctionType* type, IdentifierInfo* identifier) : type{ type },
            NamedDecl{ identifier, Decl::FunctionDeclClass }, DeclContext{ DeclContext::FunctionDeclContextClass } {
            SetDeclContext(true);
        }
        ~FunctionDecl() = default;

        inline FunctionType* GetType() const { return type; }
        inline void SetType(FunctionType* type) { this->type = type; }

        inline void SetBody(Stmt* body) { this->body = body; }
        inline Stmt* GetBody() const { return body; }

        inline bool HasFunctionFlag(FunctionFlags flag) const { return flags & flag; }
        inline void SetFunctionFlag(FunctionFlags flag) { flags = (FunctionFlags)(flags | flag); }

        inline IntrinsicID GetIntrinsicID() const { return intrinsicID; }
        inline void SetIntrinsicID(IntrinsicID intrinsicID) { this->intrinsicID = intrinsicID; }

        static inline FunctionDecl* Create(ASTContext& context, IdentifierInfo* identifier) {
            FunctionDecl* instance = context.AllocateNode<FunctionDecl>(nullptr, identifier);
            return instance;
        }

        static inline FunctionDecl* Create(ASTContext& context, IdentifierInfo* identifier, IntrinsicID intrinsicID) {
            FunctionDecl* instance = Create(context, identifier);
            instance->SetIntrinsicID(intrinsicID);
            instance->SetFunctionFlag(IsIntrinsic);
            return instance;
        }

    private:
        FunctionType* type;
        Stmt* body;

        FunctionFlags flags = None;
        IntrinsicID intrinsicID = IntrinsicID::NotIntrinsic;
    };

    class DirectiveDecl final : public Decl, private llvm::TrailingObjects<DirectiveDecl, ConstantValue*> {
        friend TrailingObjects;

    public:
        DirectiveDecl(IdentifierInfo* identifierInfo, llvm::ArrayRef<ConstantValue*> args)
                : identifierInfo{ identifierInfo }, argsCount{ args.size() }, Decl{ Decl::DirectiveDeclClass } {
            std::uninitialized_copy(args.begin(), args.end(), getTrailingObjects());
        }
        ~DirectiveDecl() = default;

        inline IdentifierInfo* GetIdentifierInfo() { return identifierInfo; }
        inline llvm::ArrayRef<ConstantValue*> GetArgs() { return { getTrailingObjects(), argsCount }; }

        static inline DirectiveDecl* Create(ASTContext& context, IdentifierInfo* identifier, llvm::ArrayRef<ConstantValue*> args, SourceRange range) {
            size_t size = totalSizeToAlloc<ConstantValue*>(args.size());
            void* ptr = context.Allocate(sizeof(DirectiveDecl) + size);
            DirectiveDecl* instance = new (ptr) DirectiveDecl{ identifier, args };
            instance->SetSourceRange(range);
            return instance;
        } 

    private:
        IdentifierInfo* identifierInfo;
        size_t argsCount;
    };

}