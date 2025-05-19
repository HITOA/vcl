/**
*   VCL Test template function
*/

in float a;
in float b;

out float r;

T Max<typename T>(T a, T b) {
    if (a > b)
        return a;
    else
        return b;
}

void Main() {
    r = Max(a, b);
}