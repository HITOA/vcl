/**
*   VCL Test Preprocessor
*/

@import "vcl/macro.vcl";

out int output;
out int outputArray;

array<int, SIZE_DEFINED_FROM_API> values = {
    0, 5, 2, 6,
    3, 9, 7, 1
};

void Main() {
    output = SOME_INT_VALUE;
    outputArray = values[INDEX_DEFINED_FROM_API];
}