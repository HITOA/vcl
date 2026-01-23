
export struct MyStruct {
    float32 f;
    uint8 i;
}

template<typename T, uint64 Size>
export struct MyTemplatedStruct {
    Array<T, Size> buff;
    int64 currentIndex;
}

template<typename T>
export T Abs(T v) {
    return v * ((v > 0) - (v < 0));
}

export float64 Sub(float32 a, float32 b) {
    return a - b;
}