@import "libmath.vcl";

out float32 o_use_helpers;
out float32 o_use_qualified;

[EntryPoint]
void Main() {
    o_use_helpers = libmath::UseHelpers<float32>((float32)10);
    o_use_qualified = libmath::UseQualified<float32>((float32)10);
}
