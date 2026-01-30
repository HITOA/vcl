// Test splat expressions (scalar to vector broadcasting)
in float32 scalar_f32;
in int32 scalar_i32;

// Output vectors from splat operations
out Vec<float32> o_splat_f32;    // Float32 splat
out Vec<int32> o_splat_i32;      // Int32 splat
out Vec<float32> o_splat_literal; // Literal splat
out Vec<float32> o_splat_expr;   // Expression splat

[EntryPoint]
void Main() {
    // Test basic splat operations
    o_splat_f32 = scalar_f32;      // Implicit splat
    o_splat_i32 = scalar_i32;      // Implicit splat
    
    // Test literal splat
    o_splat_literal = 42.0;        // Literal splat
    
    // Test expression splat
    o_splat_expr = scalar_f32 * 2.0 + 1.0;  // Expression splat
}