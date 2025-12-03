in Array<int32, 32> array;
in Span<int32> span;

in int32 index;

out Array<int32, 2> valuesOut;

void Main() {
    valuesOut[0] = array[index];
    valuesOut[1] = span[index];
}