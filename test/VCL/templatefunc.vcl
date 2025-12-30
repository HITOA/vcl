// Test template functions with various features

in float32 a;
in float32 b;
in int32 ia;
in int32 ib;

out float32 o_add;
out int32 o_iadd;
out float32 o_abs;
out float32 o_max;
out float32 o_lerp;
out int32 o_nested;
out float32 o_explicit;

// Basic template function with typename parameter
template<typename T>
T Add(T x, T y) {
    return x + y;
}

// Template function with multiple typename parameters
template<typename T, typename U>
T Cast(U value) {
    return (T)value;
}

// Template function that calls another template function
template<typename T>
T Abs(T value) {
    T zero = 0;
    return value * ((value > zero) - (value < zero));
}

// Template function with multiple parameters of the same type
template<typename T>
T Max(T x, T y) {
    return (x > y) * x + (x <= y) * y;
}

// Template function with mixed template and concrete parameters
template<typename T>
T Lerp(T start, T end, float32 t) {
    return start + (end - start) * t;
}

// Template function calling another template function (nested)
template<typename T>
T AbsMax(T x, T y) {
    T absX = Abs<T>(x);
    T absY = Abs<T>(y);
    return Max<T>(absX, absY);
}

// Template function with explicit template argument usage
template<typename T>
T Identity(T value) {
    return value;
}

void Main() {
    // Test template argument deduction
    o_add = Add(a, b);
    o_iadd = Add(ia, ib);

    // Test abs with deduction
    o_abs = Abs(a);

    // Test max
    o_max = Max(a, b);

    // Test lerp
    o_lerp = Lerp(a, b, 0.5);

    // Test nested template calls
    o_nested = AbsMax(ia, ib);

    // Test explicit template arguments
    o_explicit = Identity<float32>(a);
}
