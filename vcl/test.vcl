/**
*   TEST
*/

@import "import.vcl";
@import "import2.vcl";

[Input("Float 1")]
export float32 f1;
[Input("Int 8"), Serialize]
int8 i8 = 34;
in Span<float32> span;
out Vec<float32> vec = 2.0;
out Array<float32, 8> array = { 1.0, 2.0, 3.0, 4.0 };
import::MyStruct myStruct = { 3.1415, 163 };
import::MyTemplatedStruct<float32, 2> myTemplatedStruct = { { 1.0 }, 0 };

template<typename T>
T Abs2(T v) {
    return import::Abs<T>(v);
}

import::MyTemplatedStruct<float32, 2> Test(inout float64 value) {
    value = 2.0;
    return myTemplatedStruct;
}

void Sub2(import::MyStruct t, float32 b) {
    t.f -= b;
}

void Main() {
    Array<float32, 2> a = { f1 * import2::someValue };
    import::MyTemplatedStruct<float32, 2> b = { { 2.0 } };
    int16 f2 = (int16)f1;

    if (f2 < 10.0)
        return;
    else if (f2 == 15.0) {
        float32 f2 = ++f2;
        return;
    }

    while (1) {
        int16 f2 = ++f2;
        break;
    }

    for (int16 f2; f2 < 10; ++f2) {
        if (f2 == 3)
            continue;
    }

    f2 += Abs2(1);
    Vec<float32> v = import::Abs(-2.0);
    v += (Vec<float32>)import2::Sub(myStruct.f, 1.0);
    Sub2(myStruct, 1.0);
}