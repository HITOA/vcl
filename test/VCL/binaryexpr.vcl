// Test arithmetic binary expressions
in float32 a;
in float32 b;
in int32 ia;
in int32 ib;

// Floating point arithmetic outputs
out float32 o_add;       // Addition
out float32 o_sub;       // Subtraction  
out float32 o_mul;       // Multiplication
out float32 o_div;       // Division

// Integer arithmetic outputs
out int32 o_iadd;        // Integer addition
out int32 o_isub;        // Integer subtraction
out int32 o_imul;        // Integer multiplication
out int32 o_idiv;        // Integer division
out int32 o_irem;        // Integer remainder

// Assignment operations
out float32 o_assign;    // Direct assignment
out float32 o_add_assign; // Addition assignment
out float32 o_sub_assign; // Subtraction assignment
out float32 o_mul_assign; // Multiplication assignment
out float32 o_div_assign; // Division assignment

void Main() {
    // Basic arithmetic operations
    o_add = a + b;
    o_sub = a - b;
    o_mul = a * b;
    o_div = a / b;
    
    // Integer arithmetic operations
    o_iadd = ia + ib;
    o_isub = ia - ib;
    o_imul = ia * ib;
    o_idiv = ia / ib;
    o_irem = ia % ib;
    
    // Assignment operations
    o_assign = a;
    
    float32 temp1 = a;
    temp1 += b;
    o_add_assign = temp1;
    
    float32 temp2 = a;
    temp2 -= b;
    o_sub_assign = temp2;
    
    float32 temp3 = a;
    temp3 *= b;
    o_mul_assign = temp3;
    
    float32 temp4 = a;
    temp4 /= b;
    o_div_assign = temp4;
}