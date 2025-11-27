in Array<int32, 32> array;
in Span<int32> span;

in int32 index;

out int32 arrayOut;
out int32 spanOut;

void Main() {
    arrayOut = array[index];
    spanOut = span[index];
}