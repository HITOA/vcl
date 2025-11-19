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

// Bitwise outputs
out int32 o_band;        // Bitwise AND
out int32 o_bor;         // Bitwise OR
out int32 o_bxor;        // Bitwise XOR
out int32 o_lshift;      // Left shift
out int32 o_rshift;      // Right shift

// Comparison outputs
out bool o_greater;      // Greater than
out bool o_lesser;       // Less than
out bool o_greater_eq;   // Greater or equal
out bool o_lesser_eq;    // Less or equal
out bool o_equal;        // Equal
out bool o_not_equal;    // Not equal

// Logical outputs
out bool o_land;         // Logical AND
out bool o_lor;          // Logical OR

// Assignment operations
out float32 o_assign;    // Direct assignment
out float32 o_add_assign; // Addition assignment
out float32 o_sub_assign; // Subtraction assignment
out float32 o_mul_assign; // Multiplication assignment
out float32 o_div_assign; // Division assignment

// Integer assignment operations
out int32 o_rem_assign;    // Remainder assignment
out int32 o_band_assign;   // Bitwise AND assignment
out int32 o_bor_assign;    // Bitwise OR assignment
out int32 o_bxor_assign;   // Bitwise XOR assignment
out int32 o_lshift_assign; // Left shift assignment
out int32 o_rshift_assign; // Right shift assignment

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

    // Bitwise operations
    o_band = ia & ib;
    o_bor = ia | ib;
    o_bxor = ia ^ ib;
    o_lshift = ia << 2;
    o_rshift = ia >> 2;

    // Comparison operations
    o_greater = ia > ib;
    o_lesser = ia < ib;
    o_greater_eq = ia >= ib;
    o_lesser_eq = ia <= ib;
    o_equal = ia == ib;
    o_not_equal = ia != ib;

    // Logical operations
    o_land = (ia > 0) && (ib > 0);
    o_lor = (ia > 0) || (ib > 0);

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

    // Integer assignment operations
    int32 itemp1 = ia;
    itemp1 %= ib;
    o_rem_assign = itemp1;

    int32 itemp2 = ia;
    itemp2 &= ib;
    o_band_assign = itemp2;

    int32 itemp3 = ia;
    itemp3 |= ib;
    o_bor_assign = itemp3;

    int32 itemp4 = ia;
    itemp4 ^= ib;
    o_bxor_assign = itemp4;

    int32 itemp5 = ia;
    itemp5 <<= 2;
    o_lshift_assign = itemp5;

    int32 itemp6 = ia;
    itemp6 >>= 2;
    o_rshift_assign = itemp6;
}