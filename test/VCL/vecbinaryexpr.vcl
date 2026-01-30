// Test Vec binary expressions with different types
in Vec<float32> vf32a;
in Vec<float32> vf32b;
in Vec<float64> vf64a;
in Vec<float64> vf64b;
in Vec<int32> vi32a;
in Vec<int32> vi32b;
in Vec<bool> vba;
in Vec<bool> vbb;

// Float32 vector arithmetic operations
out Vec<float32> o_vf32_add;
out Vec<float32> o_vf32_sub;
out Vec<float32> o_vf32_mul;
out Vec<float32> o_vf32_div;

// Float32 vector comparison operations
out Vec<bool> o_vf32_greater;
out Vec<bool> o_vf32_lesser;
out Vec<bool> o_vf32_greater_eq;
out Vec<bool> o_vf32_lesser_eq;
out Vec<bool> o_vf32_equal;
out Vec<bool> o_vf32_not_equal;

// Float32 vector assignment operations
out Vec<float32> o_vf32_assign;
out Vec<float32> o_vf32_add_assign;
out Vec<float32> o_vf32_sub_assign;
out Vec<float32> o_vf32_mul_assign;
out Vec<float32> o_vf32_div_assign;

// Float64 vector arithmetic operations
out Vec<float64> o_vf64_add;
out Vec<float64> o_vf64_sub;
out Vec<float64> o_vf64_mul;
out Vec<float64> o_vf64_div;

// Float64 vector comparison operations
out Vec<bool> o_vf64_greater;
out Vec<bool> o_vf64_lesser;
out Vec<bool> o_vf64_greater_eq;
out Vec<bool> o_vf64_lesser_eq;
out Vec<bool> o_vf64_equal;
out Vec<bool> o_vf64_not_equal;

// Float64 vector assignment operations
out Vec<float64> o_vf64_assign;
out Vec<float64> o_vf64_add_assign;
out Vec<float64> o_vf64_sub_assign;
out Vec<float64> o_vf64_mul_assign;
out Vec<float64> o_vf64_div_assign;

// Int32 vector arithmetic operations
out Vec<int32> o_vi32_add;
out Vec<int32> o_vi32_sub;
out Vec<int32> o_vi32_mul;
out Vec<int32> o_vi32_div;
out Vec<int32> o_vi32_rem;

// Int32 vector bitwise operations
out Vec<int32> o_vi32_band;
out Vec<int32> o_vi32_bor;
out Vec<int32> o_vi32_bxor;
out Vec<int32> o_vi32_lshift;
out Vec<int32> o_vi32_rshift;

// Int32 vector comparison operations
out Vec<bool> o_vi32_greater;
out Vec<bool> o_vi32_lesser;
out Vec<bool> o_vi32_greater_eq;
out Vec<bool> o_vi32_lesser_eq;
out Vec<bool> o_vi32_equal;
out Vec<bool> o_vi32_not_equal;

// Int32 vector assignment operations
out Vec<int32> o_vi32_assign;
out Vec<int32> o_vi32_add_assign;
out Vec<int32> o_vi32_sub_assign;
out Vec<int32> o_vi32_mul_assign;
out Vec<int32> o_vi32_div_assign;
out Vec<int32> o_vi32_rem_assign;
out Vec<int32> o_vi32_band_assign;
out Vec<int32> o_vi32_bor_assign;
out Vec<int32> o_vi32_bxor_assign;
out Vec<int32> o_vi32_lshift_assign;
out Vec<int32> o_vi32_rshift_assign;

// Bool vector logical operations
out Vec<bool> o_vb_land;
out Vec<bool> o_vb_lor;
out Vec<bool> o_vb_equal;
out Vec<bool> o_vb_not_equal;

[EntryPoint]
void Main() {
    // Float32 vector arithmetic
    o_vf32_add = vf32a + vf32b;
    o_vf32_sub = vf32a - vf32b;
    o_vf32_mul = vf32a * vf32b;
    o_vf32_div = vf32a / vf32b;

    // Float32 vector comparisons
    o_vf32_greater = vf32a > vf32b;
    o_vf32_lesser = vf32a < vf32b;
    o_vf32_greater_eq = vf32a >= vf32b;
    o_vf32_lesser_eq = vf32a <= vf32b;
    o_vf32_equal = vf32a == vf32b;
    o_vf32_not_equal = vf32a != vf32b;

    // Float32 vector assignments
    o_vf32_assign = vf32a;
    o_vf32_add_assign = vf32a;
    o_vf32_add_assign += vf32b;
    o_vf32_sub_assign = vf32a;
    o_vf32_sub_assign -= vf32b;
    o_vf32_mul_assign = vf32a;
    o_vf32_mul_assign *= vf32b;
    o_vf32_div_assign = vf32a;
    o_vf32_div_assign /= vf32b;

    // Float64 vector arithmetic
    o_vf64_add = vf64a + vf64b;
    o_vf64_sub = vf64a - vf64b;
    o_vf64_mul = vf64a * vf64b;
    o_vf64_div = vf64a / vf64b;

    // Float64 vector comparisons
    o_vf64_greater = vf64a > vf64b;
    o_vf64_lesser = vf64a < vf64b;
    o_vf64_greater_eq = vf64a >= vf64b;
    o_vf64_lesser_eq = vf64a <= vf64b;
    o_vf64_equal = vf64a == vf64b;
    o_vf64_not_equal = vf64a != vf64b;

    // Float64 vector assignments
    o_vf64_assign = vf64a;
    o_vf64_add_assign = vf64a;
    o_vf64_add_assign += vf64b;
    o_vf64_sub_assign = vf64a;
    o_vf64_sub_assign -= vf64b;
    o_vf64_mul_assign = vf64a;
    o_vf64_mul_assign *= vf64b;
    o_vf64_div_assign = vf64a;
    o_vf64_div_assign /= vf64b;

    // Int32 vector arithmetic
    o_vi32_add = vi32a + vi32b;
    o_vi32_sub = vi32a - vi32b;
    o_vi32_mul = vi32a * vi32b;
    o_vi32_div = vi32a / vi32b;
    o_vi32_rem = vi32a % vi32b;

    // Int32 vector bitwise operations
    o_vi32_band = vi32a & vi32b;
    o_vi32_bor = vi32a | vi32b;
    o_vi32_bxor = vi32a ^ vi32b;
    o_vi32_lshift = vi32a << 2;
    o_vi32_rshift = vi32a >> 2;

    // Int32 vector comparisons
    o_vi32_greater = vi32a > vi32b;
    o_vi32_lesser = vi32a < vi32b;
    o_vi32_greater_eq = vi32a >= vi32b;
    o_vi32_lesser_eq = vi32a <= vi32b;
    o_vi32_equal = vi32a == vi32b;
    o_vi32_not_equal = vi32a != vi32b;

    // Int32 vector assignments
    o_vi32_assign = vi32a;
    o_vi32_add_assign = vi32a;
    o_vi32_add_assign += vi32b;
    o_vi32_sub_assign = vi32a;
    o_vi32_sub_assign -= vi32b;
    o_vi32_mul_assign = vi32a;
    o_vi32_mul_assign *= vi32b;
    o_vi32_div_assign = vi32a;
    o_vi32_div_assign /= vi32b;
    o_vi32_rem_assign = vi32a;
    o_vi32_rem_assign %= vi32b;
    o_vi32_band_assign = vi32a;
    o_vi32_band_assign &= vi32b;
    o_vi32_bor_assign = vi32a;
    o_vi32_bor_assign |= vi32b;
    o_vi32_bxor_assign = vi32a;
    o_vi32_bxor_assign ^= vi32b;
    o_vi32_lshift_assign = vi32a;
    o_vi32_lshift_assign <<= 2;
    o_vi32_rshift_assign = vi32a;
    o_vi32_rshift_assign >>= 2;

    // Bool vector logical operations
    o_vb_land = vba && vbb;
    o_vb_lor = vba || vbb;
    o_vb_equal = vba == vbb;
    o_vb_not_equal = vba != vbb;
}
