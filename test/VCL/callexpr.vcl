// Test function call expressions
in float32 a;
in float32 b;
in int32 ia;
in int32 ib;

// Output values for function call results
out float32 o_add_func;      // Result of Add function
out float32 o_mul_func;      // Result of Multiply function
out int32 o_abs_func;        // Result of Abs function
out float32 o_lerp_func;     // Result of Lerp function
out float32 o_nested_func;   // Result of nested function calls

// Simple arithmetic functions
float32 Add(float32 x, float32 y) {
    return x + y;
}

float32 Multiply(float32 x, float32 y) {
    return x * y;
}

// Simple absolute value function (without if statements)
int32 Abs(int32 x) {
    return x * ((x > 0) - (x < 0));  // Branchless absolute value
}

// Function with multiple parameters
float32 Lerp(float32 start, float32 end, float32 t) {
    return start + (end - start) * t;
}

// Function that calls other functions
float32 ComplexCalc(float32 x, float32 y) {
    float32 sum = Add(x, y);
    float32 product = Multiply(x, y);
    return Add(sum, product);
}

void Main() {
    // Test simple function calls
    o_add_func = Add(a, b);
    o_mul_func = Multiply(a, b);
    o_abs_func = Abs(ia);
    
    // Test function with multiple parameters
    o_lerp_func = Lerp(a, b, 0.5);
    
    // Test nested function calls
    o_nested_func = ComplexCalc(a, b);
}