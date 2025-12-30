// Test template argument deduction

in float32 a;
in float32 b;
in int32 ia;
in int32 ib;

out float32 o_deduced1;
out int32 o_deduced2;
out float32 o_deduced3;
out float32 o_multi1;
out int32 o_multi2;

// Simple template function for deduction testing
template<typename T>
T Echo(T value) {
    return value;
}

// Template function with two parameters of same type
template<typename T>
T Add(T x, T y) {
    return x + y;
}

// Template function with return type depending on parameter
template<typename T>
T Double(T value) {
    return value + value;
}

// Multiple template parameters
template<typename T, typename U>
T Convert(U value) {
    return (T)value;
}

// Template function with Vec parameter
template<typename T>
Vec<T> SplatValue(T value) {
    return (Vec<T>)value;
}

void Main() {
    // Deduction from float32
    o_deduced1 = Echo(a);

    // Deduction from int32
    o_deduced2 = Echo(ia);

    // Deduction with expression
    o_deduced3 = Double(a + b);

    // Multiple parameters requiring same type deduction
    o_multi1 = Add(a, b);
    o_multi2 = Add(ia, ib);
}
