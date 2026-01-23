/**
 *   Option 2: Direct Convolution with Cache-Optimized Memory Access
 *
 *   Best for short-to-medium impulse responses (<256 samples)
 *   Focuses on cache-friendly access patterns rather than forcing SIMD
 *   Uses a linear buffer to avoid modulo operations
 */

@grog_decl_constant ImpulseSize

in Vec<float32> input;
in Array<float32, ImpulseSize> impulse;

out Vec<float32> output = 0.0;

// Double-buffered delay line to avoid modulo in inner loop
// We maintain a linear buffer of size 2*ImpulseSize where we write twice
Array<float32, ImpulseSize * 2> delayLine = { 0.0 };
int32 writePos = 0;

void Main() {
    int32 inputLen = length(input);

    // Process each input sample
    for (int32 n = 0; n < inputLen; ++n) {
        float32 inputSample = input[n];

        // Write to both positions in double buffer
        delayLine[writePos] = inputSample;
        delayLine[writePos + ImpulseSize] = inputSample;

        // Compute convolution sum
        // This is the hot loop - compiler may auto-vectorize if conditions are right
        float32 sum = 0.0;

        // Linear access pattern - very cache friendly!
        // Modern CPUs can prefetch and auto-vectorize this
        for (int32 k = 0; k < ImpulseSize; ++k) {
            // Access is now: delayLine[writePos - k] * impulse[k]
            // which translates to: delayLine[writePos + ImpulseSize - k] due to double buffer
            sum += delayLine[writePos + ImpulseSize - k] * impulse[k];
        }

        output[n] = sum;

        // Advance write position with simple modulo
        writePos = (writePos + 1) % ImpulseSize;
    }
}
