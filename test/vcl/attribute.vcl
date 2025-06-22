/**
*   VCL Test attribute
*/

in float v1;
in float v2;

out float r;

[EntryPoint]
void Main() {
    r = v1 + v2;
}