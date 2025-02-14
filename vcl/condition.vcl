in float v1;
in float v2;
in float v3;

out float r;

void Main() {
    if (v1 > v2 && v1 > v3) r = v1;
    else if (v2 > v3) r = v2;
    else r = v3;
}