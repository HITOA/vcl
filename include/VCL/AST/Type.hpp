#pragma once

#include <llvm/ADT/FoldingSet.h>
#include <llvm/Support/TrailingObjects.h>

#include <cstdint>


namespace VCL {
    
    class TemplateTypeParamDecl;
    class TemplateDecl;
    class TemplateArgumentList;
    class RecordDecl;
    class FunctionDecl;
    class QualType;

    /**
     * Represent any kind of VCL Type in the AST and provide an interface to work with it.
     */
    class Type {
    public:
        enum TypeClass {
            BuiltinTypeClass,
            ReferenceTypeClass,
            VectorTypeClass,
            ArrayTypeClass,
            SpanTypeClass,
            RecordTypeClass,
            FunctionTypeClass,
            TemplateTypeParamTypeClass,
            TemplateSpecializationTypeClass
        };

    public:
        Type() = delete;
        Type(TypeClass typeClass) : typeClass{ typeClass } {}
        Type(const Type& other) = delete;
        Type(Type&& other) = delete;
        ~Type() = default;

        Type& operator=(const Type& other) = delete;
        Type& operator=(Type&& other) = delete;
        

        inline TypeClass GetTypeClass() const { return typeClass; }

    protected:
        TypeClass typeClass;
    };


    /**
     * Qualifier flag
     */
    enum Qualifier : uint64_t {
        None = 0x0,
        Const = 0x1,
        Mask = Const
    };

    inline Qualifier operator|(Qualifier lhs, Qualifier rhs) {
        return (Qualifier)((uint32_t)lhs | (uint32_t)rhs);
    }
    
    /**
     * Just a wrapper around a Type* and a bitfield for qualifiers
     */
    class QualType {
    public:
        QualType() = default;
        QualType(Type* type) : ptr{ (uintptr_t)type } {}
        QualType(Type* type, Qualifier qual) : ptr{ (uintptr_t)type | qual } {}
        QualType(const QualType& other) = default;
        QualType(QualType&& other) = default;

        QualType& operator=(const QualType& other) = default;
        QualType& operator=(QualType&& other) = default;
        
        inline void* GetAsOpaquePtr() const { return (void*)ptr; }
        inline Type* GetType() const { return (Type*)(ptr & ~((uintptr_t)Qualifier::Mask)); }
        inline Qualifier GetQualifiers() const { return (Qualifier)(ptr & Qualifier::Mask); }

        inline void AddQualifier(Qualifier qual) { ptr |= qual; }
        inline bool HasQualifier(Qualifier qual) { return ptr & qual; }

        inline bool operator==(const QualType& rhs) { return ptr == rhs.ptr; }
        inline bool operator!=(const QualType& rhs) { return !(*this == rhs); }

    private:
        uintptr_t ptr;
    };

    /**
     * Represent any kind of VCL Builtin Type like 'float32'.
     */
    class BuiltinType : public Type, public llvm::FoldingSetNode {
    public:
        enum Kind {
            #define BUILTIN_TYPE(id) id,
            #include <VCL/AST/BuiltinTypes.def>
            Max
        };

        enum Category {
            VoidKind,
            FloatingPointKind,
            SignedKind,
            UnsignedKind
        };

    public:
        BuiltinType(Kind kind) : kind{ kind }, Type{ Type::BuiltinTypeClass } {}
        ~BuiltinType() = default;

        inline Kind GetKind() const { return kind; }

        inline void Profile(llvm::FoldingSetNodeID& id) { Profile(id, kind);}
        inline static void Profile(llvm::FoldingSetNodeID& id, Kind kind) {
            id.AddInteger(kind);
        }

        static uint32_t GetKindBitWidth(Kind kind);
        static Category GetKindCategory(Kind kind);

    private:
        Kind kind = Kind::Void;
    };

    class ReferenceType : public Type, public llvm::FoldingSetNode {
    public:
        ReferenceType(QualType type) : type{ type }, Type{ Type::ReferenceTypeClass } {}
        ~ReferenceType() = default;
        
        inline QualType GetType() const { return type; }

        inline void Profile(llvm::FoldingSetNodeID& id) { Profile(id, type); }
        inline static void Profile(llvm::FoldingSetNodeID& id, QualType type) {
            id.AddPointer(type.GetAsOpaquePtr());
        }

    private:
        QualType type;
    };

    /**
     * Represent any kind of VCL Vector Type like 'Vec<float32>'.
     */
    class VectorType : public Type, public llvm::FoldingSetNode {
    public:
        VectorType(QualType ofType) : ofType{ ofType }, Type{ Type::VectorTypeClass } {}
        ~VectorType() = default;
        
        inline QualType GetElementType() const { return ofType; }
        
        inline void Profile(llvm::FoldingSetNodeID& id) { Profile(id, ofType);}
        inline static void Profile(llvm::FoldingSetNodeID& id, QualType ofType) {
            id.AddPointer(ofType.GetAsOpaquePtr());
        }

    private:
        QualType ofType;
    };

    /**
     * Represent any kind of VCL Array Type like 'Array<float32, 32>'
     */
    class ArrayType : public Type, public llvm::FoldingSetNode {
    public:
        ArrayType(QualType ofType, uint64_t ofSize) : ofType{ ofType }, ofSize{ ofSize }, Type{ Type::ArrayTypeClass } {}
        ~ArrayType() = default;

        inline QualType GetElementType() const { return ofType; }
        inline uint64_t GetElementCount() const { return ofSize; }
        
        inline void Profile(llvm::FoldingSetNodeID& id) { Profile(id, ofType, ofSize); }
        inline static void Profile(llvm::FoldingSetNodeID& id, QualType ofType, uint64_t ofSize) {
            id.AddPointer(ofType.GetAsOpaquePtr());
            id.AddInteger(ofSize);
        }

    private:
        QualType ofType;
        uint64_t ofSize;  
    };

    class SpanType : public Type, public llvm::FoldingSetNode {
    public:
        SpanType(QualType ofType) : ofType{ ofType }, Type{ Type::SpanTypeClass } {}
        ~SpanType() = default;

        inline QualType GetElementType() const { return ofType; }
        
        inline void Profile(llvm::FoldingSetNodeID& id) { Profile(id, ofType); }
        inline static void Profile(llvm::FoldingSetNodeID& id, QualType ofType) {
            id.AddPointer(ofType.GetAsOpaquePtr());
        }

    private:
        QualType ofType;
    };

    class RecordType : public Type, public llvm::FoldingSetNode {
    public:
        RecordType(RecordDecl* decl) : decl{ decl }, Type{ Type::RecordTypeClass } {}
        ~RecordType() = default;
        
        inline RecordDecl* GetRecordDecl() const { return decl; }

        inline void Profile(llvm::FoldingSetNodeID& id) { Profile(id, decl); }
        inline static void Profile(llvm::FoldingSetNodeID& id, RecordDecl* decl) {
            id.AddPointer(decl);
        }

    private:
        RecordDecl* decl;
    };

    class FunctionType final : public Type, public llvm::FoldingSetNode, public llvm::TrailingObjects<FunctionType, QualType> {
    public:
        friend class TrailingObjects;

        FunctionType(QualType returnType, llvm::ArrayRef<QualType> paramsType) :
                returnType{ returnType }, paramCount{ paramsType.size() }, Type{ Type::FunctionTypeClass } {
            std::uninitialized_copy(paramsType.begin(), paramsType.end(), getTrailingObjects());
        }
        ~FunctionType() = default;

        inline QualType GetReturnType() const { return returnType; }
        inline llvm::ArrayRef<QualType> GetParamsType() const {
            return { getTrailingObjects(), paramCount };
        }

        inline void Profile(llvm::FoldingSetNodeID& id) { Profile(id, returnType, GetParamsType()); }
        inline static void Profile(llvm::FoldingSetNodeID& id, QualType returnType, llvm::ArrayRef<QualType> paramsType) {
            id.AddPointer(returnType.GetAsOpaquePtr());
            for (QualType type : paramsType)
                id.AddPointer(type.GetAsOpaquePtr());
        }

    private:
        QualType returnType;
        size_t paramCount;
    };

    class TemplateTypeParamType : public Type, public llvm::FoldingSetNode {
    public:
        TemplateTypeParamType(TemplateTypeParamDecl* decl) : decl{ decl }, Type{ Type::TemplateTypeParamTypeClass } {}
        ~TemplateTypeParamType() = default;
        
        inline TemplateTypeParamDecl* GetTemplateTypeParamDecl() const { return decl; }
        
        inline void Profile(llvm::FoldingSetNodeID& id) { Profile(id, decl); }
        inline static void Profile(llvm::FoldingSetNodeID& id, TemplateTypeParamDecl* decl) {
            id.AddPointer(decl);
        }

    private:
        TemplateTypeParamDecl* decl;
    };

    class TemplateSpecializationType : public Type, public llvm::FoldingSetNode {
    public:
        TemplateSpecializationType(TemplateDecl* decl, TemplateArgumentList* args)
            : decl{ decl }, args{ args }, instantiatedType{ nullptr }, Type{ Type::TemplateSpecializationTypeClass } {}
        ~TemplateSpecializationType() = default;

        inline TemplateDecl* GetTemplateDecl() const { return decl; }
        inline TemplateArgumentList* GetTemplateArgumentList() const { return args; }
        inline Type* GetInstantiatedType() const { return instantiatedType; }
        inline void SetInstantiatedType(Type* instantiatedType) { this->instantiatedType = instantiatedType; } 

        inline void Profile(llvm::FoldingSetNodeID& id) { Profile(id, decl, args); }
        static void Profile(llvm::FoldingSetNodeID& id, TemplateDecl* decl, TemplateArgumentList* args);

    private:
        TemplateDecl* decl;
        TemplateArgumentList* args;
        Type* instantiatedType;
    };
}