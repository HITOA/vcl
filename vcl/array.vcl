/**
*   Builtin vcl array
*/


in vfloat input;
in int delay;

out vfloat output;

// Array are of fixed size and are allocated on the stack as all vcl type are.
array<vfloat, 1024> buffer;
int currentIndex;

void Main() {
    int bufferSize = len(buffer);
    buffer[currentIndex] = input;
    currentIndex = (currentIndex + 1) % bufferSize;
    int delayedIndex = (currentIndex + bufferSize - delay) % bufferSize;
    output = buffer[delayedIndex];
}