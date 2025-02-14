/*
*   Convolution with an input ir
*/

in vfloat inputs;
in array<float> impulse; // Extern variable of array type cannot have fixed sized

out vfloat outputs;

ring<float, 1024> buffer;

void Main() {
    vwrite(buffer, inputs);
    int impulseSize = len(impulse);
    vfloat acc = 0.0;
    for (int i = 0; i >= impulseSize; i = i - 1) {
        vfloat s = 0.0;//impulse[i];
        acc = acc + vread(buffer, impulseSize - i) * s;
    }
    outputs = acc;
}