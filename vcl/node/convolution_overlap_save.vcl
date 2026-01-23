/**
 *   Option 1: Block-wise Convolution using Overlap-Save method
 *
 *   This approach is best for longer impulse responses (>128 samples)
 *   Uses FFT-based convolution in frequency domain
 *
 *   Note: This is a conceptual example. Real implementation would need:
 *   - FFT/IFFT functions (could be built-in or external)
 *   - Complex number support
 *   - Proper block size management
 */

@grog_decl_constant ImpulseSize
@grog_decl_constant BlockSize  // Typically power of 2, >= ImpulseSize

in Vec<float32> input;
in Array<float32, ImpulseSize> impulse;

out Vec<float32> output = 0.0;

// Pre-computed FFT of impulse response (would be computed once at initialization)
Array<float32, BlockSize * 2> impulseFFT_real = { 0.0 };  // Real part
Array<float32, BlockSize * 2> impulseFFT_imag = { 0.0 };  // Imaginary part

// Overlap buffer to maintain state between blocks
Array<float32, ImpulseSize> overlapBuffer = { 0.0 };

// Working buffers for FFT operations
Array<float32, BlockSize * 2> fftBuffer_real = { 0.0 };
Array<float32, BlockSize * 2> fftBuffer_imag = { 0.0 };

// Simplified FFT placeholder - in practice you'd use a proper FFT library
void FFT(Array<float32, BlockSize * 2>& real, Array<float32, BlockSize * 2>& imag) {
    // TODO: Implement or call optimized FFT
    // This would use SIMD butterfly operations
    // Modern FFT libraries (like PFFFT, FFTS) are heavily SIMD-optimized
}

void IFFT(Array<float32, BlockSize * 2>& real, Array<float32, BlockSize * 2>& imag) {
    // TODO: Implement or call optimized IFFT
}

void ComplexMultiply(
    Array<float32, BlockSize * 2>& outReal,
    Array<float32, BlockSize * 2>& outImag,
    Array<float32, BlockSize * 2>& aReal,
    Array<float32, BlockSize * 2>& aImag,
    Array<float32, BlockSize * 2>& bReal,
    Array<float32, BlockSize * 2>& bImag
) {
    // Complex multiplication in frequency domain
    // (a + bi) * (c + di) = (ac - bd) + (ad + bc)i
    // This loop CAN be SIMD vectorized effectively!
    for (int32 i = 0; i < BlockSize * 2; ++i) {
        float32 ac = aReal[i] * bReal[i];
        float32 bd = aImag[i] * bImag[i];
        float32 ad = aReal[i] * bImag[i];
        float32 bc = aImag[i] * bReal[i];

        outReal[i] = ac - bd;
        outImag[i] = ad + bc;
    }
}

void Main() {
    int32 inputLen = length(input);

    // Copy input block to FFT buffer with zero padding
    for (int32 i = 0; i < inputLen && i < BlockSize; ++i) {
        fftBuffer_real[i] = input[i];
        fftBuffer_imag[i] = 0.0;
    }

    // Zero pad the rest
    for (int32 i = inputLen; i < BlockSize * 2; ++i) {
        fftBuffer_real[i] = 0.0;
        fftBuffer_imag[i] = 0.0;
    }

    // Transform input to frequency domain
    FFT(fftBuffer_real, fftBuffer_imag);

    // Complex multiply with impulse response FFT (frequency domain convolution)
    Array<float32, BlockSize * 2> resultReal;
    Array<float32, BlockSize * 2> resultImag;
    ComplexMultiply(resultReal, resultImag, fftBuffer_real, fftBuffer_imag,
                    impulseFFT_real, impulseFFT_imag);

    // Transform back to time domain
    IFFT(resultReal, resultImag);

    // Overlap-save: discard first (ImpulseSize - 1) samples, they're corrupted by circular convolution
    // Add overlap from previous block
    for (int32 i = 0; i < inputLen; ++i) {
        if (i < ImpulseSize - 1) {
            output[i] = overlapBuffer[i] + resultReal[ImpulseSize - 1 + i];
        } else {
            output[i] = resultReal[ImpulseSize - 1 + i];
        }
    }

    // Save overlap for next block
    for (int32 i = 0; i < ImpulseSize - 1; ++i) {
        overlapBuffer[i] = resultReal[BlockSize + i];
    }
}
