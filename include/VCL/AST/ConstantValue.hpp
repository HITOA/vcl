#pragma once

#include <VCL/AST/Type.hpp>

#include <cstdint>
#include <cstddef>


namespace VCL {

    class ConstantValue {
    public:
        enum ConstantValueClass {
            ConstantScalarClass
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

    private:
        template<typename T>
        inline void Set(T value) { *((T*)data) = value; }

    private:
        uint8_t data[8]{};
        BuiltinType::Kind kind = BuiltinType::Void;
    };

}