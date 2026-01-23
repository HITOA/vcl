/**
 *   Option 3b: Multi-channel SIMD Convolution
 *
 *   Process multiple audio channels simultaneously using SIMD
 *   This is where SIMD really shines - doing the same convolution
 *   on multiple independent channels at once!
 *
 *   Assumes VecWidth channels (e.g., 4 or 8 channels processed together)
 */

@grog_decl_constant ImpulseSize

// Input: each element of the vector is a sample from a different channel
// e.g., if Vec length is 4: [ch0_sample, ch1_sample, ch2_sample, ch3_sample]
in Vec<float32> inputChannels;
in Array<float32, ImpulseSize> impulse;  // Same impulse for all channels

out Vec<float32> outputChannels = 0.0;

// Delay line for each channel stored in SIMD format
// Each Array element is a Vec containing one sample from each channel
Array<Vec<float32>, ImpulseSize> delayLine;

// Initialize delay line
void Init() {
    for (int32 i = 0; i < ImpulseSize; ++i) {
        for (int32 ch = 0; ch < length(inputChannels); ++ch) {
            delayLine[i][ch] = 0.0;
        }
    }
}

void Main() {
    // Shift delay line (move samples forward in time)
    for (int32 i = ImpulseSize - 1; i > 0; --i) {
        delayLine[i] = delayLine[i - 1];
    }

    // Insert new samples (one per channel)
    delayLine[0] = inputChannels;

    // Compute convolution for all channels simultaneously
    // This is TRUE SIMD: same operation on multiple data streams!
    Vec<float32> accumulator = 0.0;  // One accumulator per channel

    for (int32 k = 0; k < ImpulseSize; ++k) {
        // Broadcast impulse coefficient and multiply with all channels
        float32 impulseCoeff = impulse[k];

        // SIMD operation: multiply all channels by same coefficient
        Vec<float32> delayed = delayLine[k];
        Vec<float32> contribution = delayed * impulseCoeff;

        // SIMD accumulation
        accumulator = accumulator + contribution;
    }

    outputChannels = accumulator;
}
