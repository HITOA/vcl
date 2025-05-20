/**
*   VCL Test span
*/

in span<float> inputs;
in int index;

out float output;
out int length;

void Main() {
    output = inputs[index];
    length = len(inputs);
}