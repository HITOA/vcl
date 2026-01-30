// Test template structs with various features

in float32 a;
in float32 b;
in int32 ia;

out float32 o_pair_first;
out float32 o_pair_second;
out float32 o_buffer_val;
out int32 o_wrapper_val;
out Vec<float32> o_nested_val;

// Basic template struct with typename parameter
template<typename T>
struct Pair {
    T first;
    T second;
}

// Template struct with non-type parameter
template<typename T, uint64 Size>
struct Buffer {
    Array<T, Size> data;
    uint64 count;
}

// Template struct with single typename parameter
template<typename T>
struct Wrapper {
    T value;
}

// Template struct with nested template types
template<typename T, uint64 Size>
struct VecBuffer {
    Array<Vec<T>, Size> vectors;
    uint64 activeCount;
}

// Function using template struct parameter
template<typename T>
T GetFirst(Pair<T> p) {
    return p.first;
}

// Function returning template struct
template<typename T>
Pair<T> MakePair(T a, T b) {
    Pair<T> result = { a, b };
    return result;
}

// Function with template struct containing non-type param
template<typename T, uint64 Size>
T GetBufferElement(Buffer<T, Size> buf, uint64 index) {
    return buf.data[index];
}

[EntryPoint]
void Main() {
    // Test basic template struct
    Pair<float32> p1 = { a, b };
    o_pair_first = p1.first;
    o_pair_second = p1.second;

    // Test template struct with non-type parameter
    Buffer<float32, 4> buf = { { a, b, 1.0, 2.0 }, 2 };
    o_buffer_val = buf.data[0];

    // Test template struct with integer type
    Wrapper<int32> wrap = { ia };
    o_wrapper_val = wrap.value;

    // Test nested template types
    VecBuffer<float32, 2> vecBuf = { { 1.0, 2.0 }, 1 };
    o_nested_val = vecBuf.vectors[0];
}
