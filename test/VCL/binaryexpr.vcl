// Test arithmetic binary expressions
in float32 a;
in float32 b;
in int32 ia;
in int32 ib;
in uint32 ua;
in uint32 ub;

// Floating point arithmetic outputs
out float32 o_add;       // Addition
out float32 o_sub;       // Subtraction
out float32 o_mul;       // Multiplication
out float32 o_div;       // Division

// Floating point comparison outputs
out bool o_fgreater;     // Float greater than
out bool o_flesser;      // Float less than
out bool o_fgreater_eq;  // Float greater or equal
out bool o_flesser_eq;   // Float less or equal
out bool o_fequal;       // Float equal
out bool o_fnot_equal;   // Float not equal

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

// Integer comparison outputs
out bool o_greater;      // Greater than
out bool o_lesser;       // Less than
out bool o_greater_eq;   // Greater or equal
out bool o_lesser_eq;    // Less or equal
out bool o_equal;        // Equal
out bool o_not_equal;    // Not equal

// Unsigned integer arithmetic outputs
out uint32 o_uadd;       // Unsigned addition
out uint32 o_usub;       // Unsigned subtraction
out uint32 o_umul;       // Unsigned multiplication
out uint32 o_udiv;       // Unsigned division
out uint32 o_urem;       // Unsigned remainder

// Unsigned bitwise outputs
out uint32 o_uband;      // Unsigned bitwise AND
out uint32 o_ubor;       // Unsigned bitwise OR
out uint32 o_ubxor;      // Unsigned bitwise XOR
out uint32 o_ulshift;    // Unsigned left shift
out uint32 o_urshift;    // Unsigned right shift

// Unsigned comparison outputs
out bool o_ugreater;     // Unsigned greater than
out bool o_uresser;      // Unsigned less than
out bool o_ugreater_eq;  // Unsigned greater or equal
out bool o_uresser_eq;   // Unsigned less or equal
out bool o_uequal;       // Unsigned equal
out bool o_unot_equal;   // Unsigned not equal

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

// Unsigned assignment operations
out uint32 o_uadd_assign;    // Unsigned addition assignment
out uint32 o_usub_assign;    // Unsigned subtraction assignment
out uint32 o_umul_assign;    // Unsigned multiplication assignment
out uint32 o_udiv_assign;    // Unsigned division assignment
out uint32 o_urem_assign;    // Unsigned remainder assignment
out uint32 o_uband_assign;   // Unsigned bitwise AND assignment
out uint32 o_ubor_assign;    // Unsigned bitwise OR assignment
out uint32 o_ubxor_assign;   // Unsigned bitwise XOR assignment
out uint32 o_ulshift_assign; // Unsigned left shift assignment
out uint32 o_urshift_assign; // Unsigned right shift assignment

void Main() {
    // Basic arithmetic operations
    o_add = a + b;
    o_sub = a - b;
    o_mul = a * b;
    o_div = a / b;

    // Floating point comparison operations
    o_fgreater = a > b;
    o_flesser = a < b;
    o_fgreater_eq = a >= b;
    o_flesser_eq = a <= b;
    o_fequal = a == b;
    o_fnot_equal = a != b;

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

    // Integer comparison operations
    o_greater = ia > ib;
    o_lesser = ia < ib;
    o_greater_eq = ia >= ib;
    o_lesser_eq = ia <= ib;
    o_equal = ia == ib;
    o_not_equal = ia != ib;

    // Unsigned integer arithmetic operations
    o_uadd = ua + ub;
    o_usub = ua - ub;
    o_umul = ua * ub;
    o_udiv = ua / ub;
    o_urem = ua % ub;

    // Unsigned bitwise operations
    o_uband = ua & ub;
    o_ubor = ua | ub;
    o_ubxor = ua ^ ub;
    o_ulshift = ua << 2;
    o_urshift = ua >> 2;

    // Unsigned comparison operations
    o_ugreater = ua > ub;
    o_uresser = ua < ub;
    o_ugreater_eq = ua >= ub;
    o_uresser_eq = ua <= ub;
    o_uequal = ua == ub;
    o_unot_equal = ua != ub;

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

    // Unsigned assignment operations
    uint32 utemp1 = ua;
    utemp1 += ub;
    o_uadd_assign = utemp1;

    uint32 utemp2 = ua;
    utemp2 -= ub;
    o_usub_assign = utemp2;

    uint32 utemp3 = ua;
    utemp3 *= ub;
    o_umul_assign = utemp3;

    uint32 utemp4 = ua;
    utemp4 /= ub;
    o_udiv_assign = utemp4;

    uint32 utemp5 = ua;
    utemp5 %= ub;
    o_urem_assign = utemp5;

    uint32 utemp6 = ua;
    utemp6 &= ub;
    o_uband_assign = utemp6;

    uint32 utemp7 = ua;
    utemp7 |= ub;
    o_ubor_assign = utemp7;

    uint32 utemp8 = ua;
    utemp8 ^= ub;
    o_ubxor_assign = utemp8;

    uint32 utemp9 = ua;
    utemp9 <<= 2;
    o_ulshift_assign = utemp9;

    uint32 utemp10 = ua;
    utemp10 >>= 2;
    o_urshift_assign = utemp10;
}