#pragma once

#include <VCL/Core/Target.hpp>

#include <llvm/Support/Allocator.h>

#include <algorithm>
#include <cstring>
#include <cstdint>
#include <cmath>


namespace VCL {

    template<typename T>
    class NumericVectorView {
    public:
        NumericVectorView(T* ptr) : ptr{ ptr } {}
        NumericVectorView(const NumericVectorView& other) = default;
        NumericVectorView(NumericVectorView&& other) = default;
        ~NumericVectorView() = default;

        inline NumericVectorView& operator=(const NumericVectorView& other) = default;
        inline NumericVectorView& operator=(NumericVectorView&& other) = default;

        inline T Get(uint32_t index) const { return ptr[index]; }
        inline void Set(uint32_t index, T value) { ptr[index] = value; }

        inline T* GetPtr() { return ptr; }
        inline bool IsValid() { return ptr != nullptr; }

        inline T& operator[](uint32_t index) { return ptr[index]; }
        inline const T& operator[](uint32_t index) const { return ptr[index]; }

    private:
        T* ptr;
    };

    using Float32VectorView = NumericVectorView<float_t>;
    using Float64VectorView = NumericVectorView<double_t>;
    using Int32VectorView = NumericVectorView<int32_t>;

    class BoolVectorView {
    public:
        BoolVectorView(uint8_t* ptr) : ptr{ ptr } {}
        BoolVectorView(const BoolVectorView& other) = default;
        BoolVectorView(BoolVectorView&& other) = default;
        ~BoolVectorView() = default;
        
        inline BoolVectorView& operator=(const BoolVectorView& other) = default;
        inline BoolVectorView& operator=(BoolVectorView&& other) = default;

        inline bool Get(uint32_t index) const {
            return ptr[index / 8] & (0b1 << (index % 8));
        }

        inline void Set(uint32_t index, bool value) {
            uint8_t m = (0b1 << (index % 8));
            ptr[index / 8] = (ptr[index / 8] & ~m) | (value ? m : 0);
        }

        inline uint8_t* GetPtr() { return ptr; }
        inline bool IsValid() const { return ptr != nullptr; }

    private:
        uint8_t* ptr;
    };

    class StorageManager {
    public:
        StorageManager(Target& target) : target{ target }, allocator{} {}
        StorageManager(const StorageManager& other) = delete;
        StorageManager(StorageManager&& other) = delete;
        ~StorageManager() = default;

        inline StorageManager& operator=(const StorageManager& other) = delete;
        inline StorageManager& operator=(StorageManager&& other) = delete;

        BoolVectorView AllocateBoolVector() {
            /** It's one bit (not byte) per boolean */
            size_t size = std::max((size_t)(target.GetVectorWidthInElement() / 8), (size_t)1);
            uint8_t* ptr = (uint8_t*)allocator.Allocate(size, target.GetVectorWidthInByte());
            return BoolVectorView{ ptr };
        }

        template<typename T>
        NumericVectorView<T> AllocateNumericVector() {
            T* ptr = (T*)allocator.Allocate(target.GetVectorWidthInElement() * sizeof(T), target.GetVectorWidthInByte());
            return NumericVectorView<T>{ ptr };
        }

        Float32VectorView AllocateFloat32Vector() { return AllocateNumericVector<float_t>(); }
        Float64VectorView AllocateFloat64Vector() { return AllocateNumericVector<double_t>(); }
        Int32VectorView AllocateInt32Vector() { return AllocateNumericVector<int32_t>(); }

    private:
        Target& target;
        llvm::BumpPtrAllocator allocator;
    };

}