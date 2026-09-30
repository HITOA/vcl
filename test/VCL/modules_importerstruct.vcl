@import "libmath.vcl";

struct Pair {
    float32 a;
    float32 b;
}

out float32 o_sum;

[EntryPoint]
void Main() {
    Pair p;
    p.a = 1.5;
    p.b = 2.0;
    o_sum = libmath::SumFields<Pair>(p);
}
