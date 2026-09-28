@import "libmath.vcl";

// Same names as libmath's: the library's templates must not pick these up.
float32 Scale = 100.0;

float32 Twice(float32 x) {
    return x * 1000.0;
}

template<typename T>
T Helper(T x) {
    return x + (T)5000;
}

out float32 o_use_helpers;

[EntryPoint]
void Main() {
    o_use_helpers = libmath::UseHelpers<float32>((float32)10);
}
