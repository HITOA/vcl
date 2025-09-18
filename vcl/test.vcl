/**
*   TEST
*/

struct MyStruct {
    float32 f;
    int8 i;
}

struct MyTemplatedStruct<typename T, int64 Size> {
    Array<T, Size> buff;
    int64 currentIndex;
}

float32 f1 = 0.0;
Array<float32, 4 * 8> array;
MyStruct myStruct;
MyTemplatedStruct<float32, 1024> myTemplatedStruct;