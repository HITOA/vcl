/**
*   Generic ring buffer
*/

struct Ring<typename T, int Size> {
    array<T, Size> buffer;
    int currentIndex;
}

void Main() {

}