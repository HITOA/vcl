/**
*   TEST
*/

struct MyStruct {
    float32 f;
    uint8 i;
}

template<typename T, uint64 Size>
struct MyTemplatedStruct {
    Array<T, Size> buff;
    int64 currentIndex;
}

float32 f1;
int8 i8 = 34;
in Span<float32> span;
out Vec<float32> vec = 2.0;
out Array<float32, 8> array = { 1.0, 2.0, 3.0, 4.0 };
MyStruct myStruct = { 3.1415, 163 };
MyTemplatedStruct<float32, 2> myTemplatedStruct = { { 1.0 }, 0 };

template<typename T>
T Abs(T v) {
    return v * ((v > 0) - (v < 0));
}

template<typename T>
T Abs2(T v) {
    return Abs<T>(v);
}

MyTemplatedStruct<float32, 2> Test(inout float64 value) {
    value = 2.0;
    return myTemplatedStruct;
}

float64 Sub(float32 a, float32 b) {
    return a - b;
}

void Sub2(MyStruct t, float32 b) {
    t.f -= b;
}

void Main() {
    Array<float32, 2> a = { f1 * 2.0 };
    MyTemplatedStruct<float32, 2> b = { { 2.0 } };
    int16 f2 = (int16)f1;

    if (f2 < 10.0)
        return;
    else if (f2 == 15.0) {
        float32 f2 = ++f2;
        return;
    }
    
    f2 += Abs2(1);
    Vec<float32> v = Abs(-2.0);
    v += (Vec<float32>)Sub(myStruct.f, 1.0);
    Sub2(myStruct, 1.0);
}