/**
*   VCL Test template ring
*/

struct Ring<typename T, int Size> {
    array<T, Size> buffer;
    int currentIndex;
}

T ReadRing<typename T, int Size>(Ring<T, Size> ring, int offset) {
    int index = (ring.currentIndex + Size - offset) % Size;
    return ring.buffer[index];
}

void WriteRing<typename T, int Size>(Ring<T, Size> ring, T value) {
    ring.buffer[ring.currentIndex] = value;
    ring.currentIndex = (ring.currentIndex + 1) % Size;
}

in vfloat input;
in int delay;

out vfloat output;

Ring<vfloat, 1024> delayLine;

void Main() {
    WriteRing(delayLine, input);
    output = ReadRing(delayLine, delay);
}