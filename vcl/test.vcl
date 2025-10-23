/**
*   TEST
*/

struct MyStruct {
    float32 f;
    int8 i;
}

struct MyTemplatedStruct<typename T, uint64 Size> {
    Array<T, Size> buff;
    int64 currentIndex;
}

in float32 f1;
int8 i8 = 34;
in Span<float32> span;
out Array<float32, 8> array;
out Vec<float32> vec = 2.0;
MyStruct myStruct;
MyTemplatedStruct<float32, 1024> myTemplatedStruct;

MyTemplatedStruct<float32, 1024> Test(inout float64 value) {
    value = 2.0;
    return myTemplatedStruct;
}

float32 Sub(float32 a, float32 b) {
    return a - b;
}

void Sub2(MyStruct t, float32 b) {
    t.f -= b;
}

void Main() {
    int16 f2 = f1;
    f2 += 1;
    Vec<float32> v = 5.0;
    v += Sub(myStruct.f, 1.0);
    Sub2(myStruct, 1.0);
}