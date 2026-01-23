/**
*   templated sized convolution node example
*/

@grog_decl_constant Size

in Vec<float32> input;
in Array<float32, Size> impulse; // When linking this port to the output of another node, Size will be substituted with the array size of the linked output port

out Vec<float32> output = 0.0;

Array<float32, Size> delay = { 0.0 };
int32 index = 0;

void DelayWrite(float32 value) {
    delay[index] = value;
    index = (index + 1) % Size;
}

float32 DelayRead(int32 offset) {
    int32 readIndex = (index + Size - offset) % Size;
    return delay[readIndex];
}

void Main() {
    for (int32 i = 0; i < length(input); ++i) {
        DelayWrite(input[i]);
        for (int32 j = 0; j < Size; ++j) {
            float32 impulseSample = impulse[j];
            float32 delayedSample = DelayRead(Size - 1 - j);
            output[i] += delayedSample * impulseSample;
        }
    }
}