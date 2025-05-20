/**
*   VCL Test on array
*/

in int input;

out int output;

array<int, 8> buffer;
int currentIndex = 0;

void Main() {
    buffer[currentIndex] = input;
    currentIndex = (currentIndex + 1) % 8;
    output = buffer[currentIndex];
}