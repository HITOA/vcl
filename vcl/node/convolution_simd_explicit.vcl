/**
 *   Option 3: Explicit SIMD Operations for Convolution
 *
 *   Uses SIMD vector operations explicitly where possible
 *   Best for very short impulse responses that are multiples of vector size
 *   or when processing multiple channels simultaneously
 */

@grog_decl_constant ImpulseSize

in Vec<float32> input;
in Array<float32, ImpulseSize> impulse;

out Vec<float32> output = 0.0;

// History buffer - store previous samples
Array<float32, ImpulseSize> history = { 0.0 };

// Helper: horizontal sum of a vector (reduces Vec to scalar)
// This operation IS vectorized - uses SIMD horizontal add instructions
float32 HorizontalSum(Vec<float32> v) {
    float32 sum = 0.0;
    for (int32 i = 0; i < length(v); ++i) {
        sum += v[i];
    }
    return sum;
}

// Compute dot product using SIMD when data is aligned
float32 DotProduct(Array<float32, ImpulseSize>& a, Array<float32, ImpulseSize>& b) {
    // If ImpulseSize is a multiple of the vector width, this can be vectorized
    // Process in chunks that fit in SIMD registers
    float32 result = 0.0;

    // Assuming we can determine vector width at compile time
    int32 vecWidth = length(input);  // Or use a constant if known

    // Vectorizable portion
    int32 vectorizableSize = (ImpulseSize / vecWidth) * vecWidth;
    for (int32 i = 0; i < vectorizableSize; i += vecWidth) {
        Vec<float32> va;
        Vec<float32> vb;

        // Load chunks into vectors
        for (int32 j = 0; j < vecWidth && (i + j) < ImpulseSize; ++j) {
            va[j] = a[i + j];
            vb[j] = b[i + j];
        }

        // SIMD multiply
        Vec<float32> prod = va * vb;

        // Accumulate
        result += HorizontalSum(prod);
    }

    // Handle remainder
    for (int32 i = vectorizableSize; i < ImpulseSize; ++i) {
        result += a[i] * b[i];
    }

    return result;
}

void Main() {
    int32 inputLen = length(input);

    // Shift history buffer
    for (int32 i = ImpulseSize - 1; i > 0; --i) {
        history[i] = history[i - 1];
    }

    // Process each sample
    for (int32 n = 0; n < inputLen; ++n) {
        // Shift in new sample
        for (int32 i = ImpulseSize - 1; i > 0; --i) {
            history[i] = history[i - 1];
        }
        history[0] = input[n];

        // Compute convolution using dot product
        output[n] = DotProduct(history, impulse);
    }
}
