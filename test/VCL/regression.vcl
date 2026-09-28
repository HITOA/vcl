// Regression tests for bugs found in the 2026-09 review (see Grog docs/vcl-review.md).

in int32 ri32;
in uint32 ru32;
in float32 rf32;

// C1: right shift of unsigned values is logical.
out uint32 o_shr_unsigned;
out int32 o_shr_signed;

// C2: abs
out float32 o_abs_f32;
out int32 o_abs_i32;

// C3: min / max
out int32 o_min_i32;
out uint32 o_max_u32;
out float32 o_min_f32;

// C4: && and || short-circuit.
out int32 o_calls;
out bool o_and_result;
out bool o_or_result;

// C5: integer literals are at least int32.
out int32 o_literal_mul;
out int64 o_literal_big;

// C6: explicit casts to a template parameter survive instantiation.
out float32 o_dependent_cast;

// C7: pack/unpack of a Lanes field that isn't vector-aligned.
out float32 o_misaligned_lanes_sum;

// C8: statements after return.
out int32 o_after_return;

// Parser: `a < b || c > d` is a comparison, not a template argument list.
out int32 o_comparison_not_template;

// P1: locals declared in a loop body.
out int32 o_loop_locals;

// T2: template arguments are compared canonically (an alias equals the type it names).
out float32 o_special_via_alias;
out float32 o_id_alias;
out float32 o_id_direct;

bool Touch() {
    o_calls += 1;
    return 1 == 1;
}

template<typename T>
T Ratio(int32 a, int32 b) {
    return (T)a / (T)b;
}

struct Misaligned {
    float32 gain;
    Lanes<float32> lanes;
}

Misaligned misaligned;

using Real = float32;

template<typename T>
T Triple(T x) {
    return x * (T)2; // deliberately wrong: the specialization below must be used instead
}

special<Real>
Real Triple(Real x) {
    return x * (Real)3;
}

template<typename T>
T Id(T x) {
    return x;
}

int32 EarlyReturn() {
    return 7;
    o_after_return = 1;
}

[EntryPoint]
void Main() {
    o_shr_unsigned = ru32 >> 1;
    o_shr_signed = ri32 >> 1;

    o_abs_f32 = abs(rf32);
    o_abs_i32 = abs(ri32);

    o_min_i32 = min(ri32, 3);
    o_max_u32 = max(ru32, (uint32)5);
    o_min_f32 = min(rf32, (float32)0.25);

    o_calls = 0;
    o_and_result = ri32 > 1000000 && Touch();
    o_or_result = ri32 < 1000000 || Touch();

    o_literal_mul = 100 * 3;
    o_literal_big = 5000000000;

    o_dependent_cast = Ratio<float32>(1, 2);

    for (uint32 i = 0; i < length(misaligned.lanes); ++i)
        misaligned.lanes[i] = (float32)i;
    Vec<float32> packed = pack<float32>(misaligned.lanes);
    Lanes<float32> unpacked = unpack<float32>(packed * 2.0);
    o_misaligned_lanes_sum = 0.0;
    for (uint32 i = 0; i < length(unpacked); ++i)
        o_misaligned_lanes_sum += unpacked[i];

    o_after_return = EarlyReturn();

    o_special_via_alias = Triple<float32>((float32)1);
    o_id_alias = Id<Real>((float32)4);
    o_id_direct = Id<float32>((float32)5);

    int32 t = 5;
    int32 T = 3;
    if (t < 0 || t > T)
        o_comparison_not_template = 1;
    else
        o_comparison_not_template = 0;

    o_loop_locals = 0;
    for (int32 i = 0; i < 4; ++i) {
        int32 doubled = i * 2;
        o_loop_locals += doubled;
    }
}
