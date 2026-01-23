#pragma once

#include <VCL/Core/Identifier.hpp>
#include <VCL/AST/Type.hpp>

#include <cstdint>
#include <cstddef>


namespace VCL {

    class ConstantValue {
    public:
        enum ConstantValueClass {
            ConstantScalarClass,
            ConstantStringClass,
            ConstantIdentifierClass,
            ConstantAggregateClass,
            ConstantNullClass
        };

    public:
        ConstantValue() = delete;
        ConstantValue(ConstantValueClass constantValueClass) : constantValueClass{ constantValueClass } {}
        ConstantValue(const ConstantValue& other) = default;
        ConstantValue(ConstantValue&& other) = default;
        ~ConstantValue() = default;

        ConstantValue& operator=(const ConstantValue& other) = default;
        ConstantValue& operator=(ConstantValue&& other) = default;

        inline ConstantValueClass GetConstantValueClass() const { return constantValueClass; }

    private:
        ConstantValueClass constantValueClass;
    };

    class ConstantScalar : public ConstantValue {
    public:
        ConstantScalar() : ConstantValue{ ConstantValue::ConstantScalarClass } {}
        ConstantScalar(float v) : ConstantValue{ ConstantValue::ConstantScalarClass } { *this = v; }
        ConstantScalar(double v) : ConstantValue{ ConstantValue::ConstantScalarClass } { *this = v; }
        ConstantScalar(int8_t v) : ConstantValue{ ConstantValue::ConstantScalarClass } { *this = v; }
        ConstantScalar(int16_t v) : ConstantValue{ ConstantValue::ConstantScalarClass } { *this = v; }
        ConstantScalar(int32_t v) : ConstantValue{ ConstantValue::ConstantScalarClass } { *this = v; }
        ConstantScalar(int64_t v) : ConstantValue{ ConstantValue::ConstantScalarClass } { *this = v; }
        ConstantScalar(uint8_t v) : ConstantValue{ ConstantValue::ConstantScalarClass } { *this = v; }
        ConstantScalar(uint16_t v) : ConstantValue{ ConstantValue::ConstantScalarClass } { *this = v; }
        ConstantScalar(uint32_t v) : ConstantValue{ ConstantValue::ConstantScalarClass } { *this = v; }
        ConstantScalar(uint64_t v) : ConstantValue{ ConstantValue::ConstantScalarClass } { *this = v; }
        ~ConstantScalar() = default;
        
        template<typename T>
        inline T Get() const { return *((T*)data); }

        inline BuiltinType::Kind GetKind() const { return kind; }

        inline ConstantScalar& operator=(float v) {
            Set<float>(v);
            kind = BuiltinType::Float32;
            return *this;
        }

        inline ConstantScalar& operator=(double v) {
            Set<double>(v);
            kind = BuiltinType::Float64;
            return *this;
        }

        inline ConstantScalar& operator=(int8_t v) {
            Set<int8_t>(v);
            kind = BuiltinType::Int8;
            return *this;
        }

        inline ConstantScalar& operator=(int16_t v) {
            Set<int16_t>(v);
            kind = BuiltinType::Int16;
            return *this;
        }

        inline ConstantScalar& operator=(int32_t v) {
            Set<int32_t>(v);
            kind = BuiltinType::Int32;
            return *this;
        }

        inline ConstantScalar& operator=(int64_t v) {
            Set<int64_t>(v);
            kind = BuiltinType::Int64;
            return *this;
        }

        inline ConstantScalar& operator=(uint8_t v) {
            Set<uint8_t>(v);
            kind = BuiltinType::UInt8;
            return *this;
        }

        inline ConstantScalar& operator=(uint16_t v) {
            Set<uint16_t>(v);
            kind = BuiltinType::UInt16;
            return *this;
        }

        inline ConstantScalar& operator=(uint32_t v) {
            Set<uint32_t>(v);
            kind = BuiltinType::UInt32;
            return *this;
        }

        inline ConstantScalar& operator=(uint64_t v) {
            Set<uint64_t>(v);
            kind = BuiltinType::UInt64;
            return *this;
        }

        inline bool operator==(const ConstantScalar& rhs) { return kind == rhs.kind && memcmp(data, rhs.data, sizeof(data)) == 0; }
        inline bool operator!=(const ConstantScalar& rhs) { return !(*this == rhs); }

    private:
        template<typename T>
        inline void Set(T value) { 
            memset(data, 0x0, sizeof(uint8_t) * 8);
            memcpy(data, &value, sizeof(value));
        }

    private:
        uint8_t data[8]{};
        BuiltinType::Kind kind = BuiltinType::Void;
    };

    class ConstantString : public ConstantValue {
    public:
        ConstantString(llvm::StringRef str) : str{ str.str() }, ConstantValue{ ConstantValue::ConstantStringClass } {}
        ~ConstantString() = default;

        inline std::string GetString() const { return str; }
        
    private:
        std::string str{};
    };
    
    class ConstantIdentifier : public ConstantValue {
    public:
        ConstantIdentifier(IdentifierInfo* identifierInfo) : identifierInfo{ identifierInfo }, ConstantValue{ ConstantValue::ConstantIdentifierClass } {}
        ~ConstantIdentifier() = default;
        
        inline IdentifierInfo* GetIdentifierInfo() const { return identifierInfo; }

    private:
        IdentifierInfo* identifierInfo;
    };

    class ConstantAggregate : public ConstantValue {
    public:
        ConstantAggregate(llvm::ArrayRef<ConstantValue*> values, QualType type) 
                : values{ values }, type{ type }, ConstantValue{ ConstantValue::ConstantAggregateClass } {}
        ~ConstantAggregate() = default;

        llvm::ArrayRef<ConstantValue*> GetValues() const { return { values.begin(), values.size() }; }
        ConstantValue* GetValue(size_t index) { return values[index]; }
        size_t GetValueCount() const { return values.size(); }

        QualType GetType() { return type; }

    private:
        llvm::SmallVector<ConstantValue*> values{};
        QualType type{};
    };

    class ConstantNull : public ConstantValue {
    public:
        ConstantNull(QualType type) : type{ type }, ConstantValue{ ConstantValue::ConstantNullClass } {}
        ~ConstantNull() = default;
        
        QualType GetType() { return type; }

    private:
        QualType type{};
    };

}