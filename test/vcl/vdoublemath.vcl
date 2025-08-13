/**
*   VCL Test vdouble math
*/

in vdouble a;
in vfloat b;
in float c;
in double d;

out vdouble result;


void Main() {
    result = (a + d) * a * 2 * b * c;
}