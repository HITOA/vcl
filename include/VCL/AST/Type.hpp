#pragma once

#include <llvm/ADT/FoldingSet.h>

#include <cstdint>


namespace VCL {

    /**
     * Represent any kind of VCL Type in the AST and provide an interface to work with it.
     */
    class Type {
    public:
        enum TypeClass {
            BuiltinType,
            VectorType,
            ArrayType,
            TemplateTypeParamType
        };

    public:
        Type() = delete;
        Type(TypeClass typeClass) : typeClass{ typeClass } {}
        Type(const Type& other) = delete;
        Type(Type&& other) = delete;
        ~Type() = default;

        Type& operator=(const Type& other) = delete;
        Type& operator=(Type&& other) = delete;
        

        inline TypeClass GetTypeClass() const {
            return typeClass;
        }

    protected:
        TypeClass typeClass;
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

    public:
        BuiltinType() = delete;
        BuiltinType(Kind kind) : kind{ kind }, Type{ Type::BuiltinType } {}
        BuiltinType(const BuiltinType& other) = delete;
        BuiltinType(BuiltinType&& other) = delete;
        ~BuiltinType() = default;

        BuiltinType& operator=(const BuiltinType& other) = delete;
        BuiltinType& operator=(BuiltinType&& other) = delete;


        inline void Profile(llvm::FoldingSetNodeID& id) { Profile(id, kind);}
        inline static void Profile(llvm::FoldingSetNodeID& id, Kind kind) {
            id.AddInteger(kind);
        }

    private:
        Kind kind = Kind::Void;
    };

    /**
     * Represent any kind of VCL Vector Type like 'Vec<float32>'.
     */
    class VectorType : public Type, public llvm::FoldingSetNode {
    public:
        VectorType() = delete;
        VectorType(Type* ofType) : ofType{ ofType }, Type{ Type::VectorType } {}
        VectorType(const VectorType& other) = delete;
        VectorType(VectorType&& other) = delete;
        ~VectorType() = default;

        VectorType& operator=(const VectorType& other) = delete;
        VectorType& operator=(VectorType&& other) = delete;
        

        
        inline void Profile(llvm::FoldingSetNodeID& id) { Profile(id, ofType);}
        inline static void Profile(llvm::FoldingSetNodeID& id, Type* ofType) {
            id.AddPointer(ofType);
        }

    private:
        Type* ofType = nullptr;
    };

    /**
     * Represent any kind of VCL Array Type like 'Array<float32, 32>'
     */
    class ArrayType : public Type, public llvm::FoldingSetNode {
    public:
        ArrayType() = delete;
        ArrayType(Type* ofType, uint64_t ofSize) : ofType{ ofType }, ofSize{ ofSize }, Type{ Type::ArrayType } {}
        ArrayType(const ArrayType& other) = delete;
        ArrayType(ArrayType&& other) = delete;
        ~ArrayType() = default;

        ArrayType& operator=(const ArrayType& other) = delete;
        ArrayType& operator=(ArrayType&& other) = delete;
        

        
        inline void Profile(llvm::FoldingSetNodeID& id) { Profile(id, ofType, ofSize);}
        inline static void Profile(llvm::FoldingSetNodeID& id, Type* ofType, uint64_t ofSize) {
            id.AddPointer(ofType);
            id.AddInteger(ofSize);
        }

    private:
        Type* ofType = nullptr;
        uint64_t ofSize = 0;  
    };

    /**
     * Represent a template type parameter
     */
    class TemplateTypeParamType : public Type, public llvm::FoldingSetNode {

    };

}