/**
*   VCL Test extract & insert
*/

in vfloat a;
in vdouble b;

out vdouble result;

void Main() {
    for (int i = 0; i < len(result); ++i) {
        insert(result, i, extract(a, i) * extract(b, i) / (i + 1));
    }
}