// Test unary expressions
in int32 ia;
in uint32 ua;
in bool ba;

// Unary operation outputs
out int32 o_prefix_inc;      // Prefix increment (++x)
out int32 o_prefix_dec;      // Prefix decrement (--x)
out int32 o_plus;            // Unary plus (+x)
out int32 o_minus;           // Unary minus (-x)
out bool o_logical_not;      // Logical NOT (!x)
out uint32 o_bitwise_not;    // Bitwise NOT (~x)
out int32 o_postfix_inc;     // Postfix increment (x++)
out int32 o_postfix_dec;     // Postfix decrement (x--)

// Result values after increment/decrement operations
out int32 o_prefix_inc_result;   // Value after prefix increment
out int32 o_prefix_dec_result;   // Value after prefix decrement
out int32 o_postfix_inc_result;  // Value after postfix increment
out int32 o_postfix_dec_result;  // Value after postfix decrement

[EntryPoint]
void Main() {
    // Prefix increment: increments value and returns new value
    int32 temp1 = ia;
    o_prefix_inc = ++temp1;
    o_prefix_inc_result = temp1;

    // Prefix decrement: decrements value and returns new value
    int32 temp2 = ia;
    o_prefix_dec = --temp2;
    o_prefix_dec_result = temp2;

    // Unary plus: identity operation
    o_plus = +ia;

    // Unary minus: negation
    o_minus = -ia;

    // Logical NOT
    o_logical_not = !ba;

    // Bitwise NOT
    o_bitwise_not = ~ua;

    // Postfix increment: returns original value, then increments
    int32 temp3 = ia;
    o_postfix_inc = temp3++;
    o_postfix_inc_result = temp3;

    // Postfix decrement: returns original value, then decrements
    int32 temp4 = ia;
    o_postfix_dec = temp4--;
    o_postfix_dec_result = temp4;
}
