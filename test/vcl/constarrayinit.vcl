/**
*   VCL Test const aggregate initializer for array
*/

in int index;

out float output;

array<float, 16> values = {
    4.92, 8.84, 9.28, 5.45,
    9.66, 1.53, 9.73, 3.68,
    6.93, 4.51, 5.65, 3.89,
    1.65, 5.23, 8.84, 5.67
};

void Main() {
    output = values[index];
}