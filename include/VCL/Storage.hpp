#pragma once

#include <VCL/NativeTarget.hpp>

#include <cstring>

#ifdef _MSC_VER
    #define ALIGNED_MALLOC(size, align) _aligned_malloc(size, align)
    #define ALIGNED_FREE(ptr) _aligned_free(ptr)
#else
    #define ALIGNED_MALLOC(size, align) aligned_alloc(align, size)
    #define ALIGNED_FREE(ptr) free(ptr)
#endif

/**
 * Helpers to deal and create correctly sized and aligned storage for VCL symbol.
 */
namespace VCL {

    /**
     * Base Storage class
     */
    class Storage {
    public:
        virtual ~Storage() {};

        virtual void* GetPtr() = 0;
    };

    /**
     * Allocate storage for any given type.
     */
    template<typename T>
    class TypedStorage : public Storage {
    public:
        TypedStorage() : ptr{ (T*)malloc(sizeof(T)) } {}
        TypedStorage(const T& value) : ptr{ (T*)malloc(sizeof(T)) } {
            *ptr = value;
        }
        TypedStorage(T&& value) : ptr{ (T*)malloc(sizeof(T)) } {
            *ptr = value;
        }
        TypedStorage(const TypedStorage<T>&) = delete;
        TypedStorage(TypedStorage<T>&& other) : ptr{ std::move(other.ptr) } { other.ptr = nullptr; }
        virtual ~TypedStorage() override { 
            if (ptr != nullptr)
                free(ptr); 
        }    

        TypedStorage& operator=(const TypedStorage&) = delete;
        TypedStorage& operator=(TypedStorage&& other) { 
            if (ptr != nullptr)
                free(ptr);
            ptr = std::move(other.ptr);
            other.ptr = nullptr;    
        }

        virtual void* GetPtr() override { return ptr; }
        
        const T& Get() const { return *ptr; }
        void Set(const T& value) { *ptr = value; }
        void Set(T&& value) { *ptr = value; }

    private:
        T* ptr;
    };

    /**
     * Allocate storage for any given vector type.
     */
    template<typename T>
    class VectorTypedStorage : public Storage {
    public:
        VectorTypedStorage() : 
            elemCount{ NativeTarget::GetInstance()->GetMaxVectorElementWidth() },
            ptr{ (T*)ALIGNED_MALLOC(sizeof(T) * elemCount, NativeTarget::GetInstance()->GetMaxVectorByteWidth()) } {}
        VectorTypedStorage(const T& value) : 
            elemCount{ NativeTarget::GetInstance()->GetMaxVectorElementWidth() },
            ptr{ (T*)ALIGNED_MALLOC(sizeof(T) * elemCount, NativeTarget::GetInstance()->GetMaxVectorByteWidth()) } {
            for (uint32_t i = 0; i < elemCount; ++i)
                ptr[i] = value;
        }
        VectorTypedStorage(T&& value) : 
            elemCount{ NativeTarget::GetInstance()->GetMaxVectorElementWidth() },
            ptr{ (T*)ALIGNED_MALLOC(sizeof(T) * elemCount, NativeTarget::GetInstance()->GetMaxVectorByteWidth()) } {
            for (uint32_t i = 0; i < elemCount; ++i)
                ptr[i] = value;
        }
        VectorTypedStorage(const VectorTypedStorage<T>&) = delete;
        VectorTypedStorage(VectorTypedStorage<T>&& other) : elemCount{ std::move(other.elemCount) }, ptr{ std::move(other.ptr) } { other.ptr = nullptr; }
        virtual ~VectorTypedStorage() override { 
            if (ptr != nullptr)
                ALIGNED_FREE(ptr); 
        }    

        VectorTypedStorage& operator=(const VectorTypedStorage&) = delete;
        VectorTypedStorage& operator=(VectorTypedStorage&& other) { 
            if (ptr != nullptr)
                ALIGNED_FREE(ptr);
            elemCount = std::move(other.elemCount);
            ptr = std::move(other.ptr);
            other.ptr = nullptr;    
        }

        virtual void* GetPtr() override { return ptr; }

        uint32_t GetElementCount() const { return elemCount; }

        void Set(const T& value) {
            for (uint32_t i = 0; i < elemCount; ++i)
                ptr[i] = value;
        }
        void Set(T&& value) {
            for (uint32_t i = 0; i < elemCount; ++i)
                ptr[i] = value;
        }
        void SetValues(T* values, uint32_t count) {
            memcpy(ptr, values, count > elemCount ? elemCount : count);
        }
        void SetValues(T* begin, T* end) {
            uint32_t count = (end - begin) / sizeof(T);
            SetValues(begin, count);
        }

        const T& GetValue(uint32_t index) { return ptr[index]; }

        T& operator[](size_t index) { return ptr[index]; }
        const T& operator[](size_t index) const { return ptr[index]; }

    private:
        uint32_t elemCount;
        T* ptr;
    };

    using FloatStorage = TypedStorage<float>;
    using BoolStorage = TypedStorage<bool>;
    using IntStorage = TypedStorage<int>;
    using DoubleStorage = TypedStorage<double>;

    using VectorFloatStorage = VectorTypedStorage<float>;
    using VectorBoolStorage = VectorTypedStorage<bool>;
    using VectorIntStorage = VectorTypedStorage<int>;
    using VectorDoubleStorage = VectorTypedStorage<double>;
}