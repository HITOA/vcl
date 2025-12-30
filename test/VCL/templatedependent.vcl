// Test dependent expressions in templates

in float32 a;
in float32 b;
in int32 ia;

out float32 o_field_access;
out float32 o_binary_op;
out float32 o_unary_op;
out float32 o_call_chain;
out float32 o_subscript;
out float32 o_cast;

// Template struct with fields
template<typename T>
struct Container {
    T value;
    T backup;
}

// Template function with field access on dependent type
template<typename T>
T GetValue(Container<T> c) {
    return c.value;
}

// Template function with binary operations on dependent types
template<typename T>
T Compute(T x, T y) {
    T sum = x + y;
    T prod = x * y;
    T result = sum - prod;
    return result;
}

// Template function with unary operations
template<typename T>
T Negate(T value) {
    T result = -value;
    return result;
}

// Template function calling another template function (dependent call)
template<typename T>
T ProcessValue(T value) {
    T neg = Negate<T>(value);
    T doubled = neg + neg;
    return doubled;
}

// Template function with array subscript
template<typename T, uint64 Size>
T GetElement(Array<T, Size> arr, uint64 index) {
    return arr[index];
}

// Template function with cast
template<typename T, typename U>
T CastValue(U value) {
    T result = (T)value;
    return result;
}

// Helper function
template<typename T>
T Helper(T val) {
    return val * 2;
}

// Chained template calls
template<typename T>
T ChainedCall(T value) {
    return Helper<T>(Helper<T>(value));
}

void Main() {
    // Test dependent field access
    Container<float32> cont = { a, b };
    o_field_access = GetValue<float32>(cont);

    // Test dependent binary operations
    o_binary_op = Compute(a, b);

    // Test dependent unary operations
    o_unary_op = Negate(a);

    // Test dependent call chain
    o_call_chain = ChainedCall(a);

    // Test dependent subscript
    Array<float32, 3> arr = { a, b, 1.0 };
    o_subscript = GetElement<float32, 3>(arr, 0);

    // Test dependent cast
    o_cast = CastValue<float32, int32>(ia);
}
