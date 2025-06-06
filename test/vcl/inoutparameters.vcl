/**
*   VCL Test in out parameters
*/

struct StructFloat {
    float v;
}

void DefaultQualifier(float a, StructFloat b) {
    a = 1.0;
    b.v = 1.0;
}

void InQualifier(in float a, in StructFloat b) {
    a = 1.0;
    b.v = 1.0;
}

void OutQualifier(out float a, out StructFloat b) {
    a = 1.0;
    b.v = 1.0;
}

in float a1;
in float a2;
in float a3;
in StructFloat b1;
in StructFloat b2;
in StructFloat b3;

void Main() {
    DefaultQualifier(a1, b1);
    InQualifier(a2, b2);
    OutQualifier(a3, b3);
}