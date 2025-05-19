/**
*   VCL Test const
*/

const int v = 5;

void Main() {
    v = 0; // Should throw an error while emitting IR
}