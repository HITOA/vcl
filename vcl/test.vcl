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

float32 f1 = 0.0;
Array<float32, 8> array;
Vec<float32> vec;
//Delegate<void()> d1;
//Delegate<int(int, int)> d2;
MyStruct myStruct;
MyTemplatedStruct<float32, 1024> myTemplatedStruct;