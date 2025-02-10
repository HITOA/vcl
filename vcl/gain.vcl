in vfloat left;
in vfloat right;

in float gain;

out vfloat outLeft;
out vfloat outRight;

void Main() {
    outLeft = left * gain;
    outRight = right * gain;
}