/**
*   VCL Test const to non const error
*/

const int v = 0;

void SomeFunction(ref int v) {
    v = 5;
}

void Main() {
    SomeFunction(v); // Should throw an error because v is const
}