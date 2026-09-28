@import "libbroken.vcl";
out float32 o;
[EntryPoint]
void Main() {
    o = libbroken::Broken<float32>((float32)1);
}
