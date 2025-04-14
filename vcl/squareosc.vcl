/*
*   Square wave generator with PolyBLEP
*/

in vfloat time;

in float frequency;
in float velocity;
in float rate;

out vfloat outLeft;
out vfloat outRight;

vfloat PolyBlep(vfloat p, float dt) {
    vfloat p1 = p / dt;
    vfloat v1 = p1 * 2.0 - p1 * p1 - 1.0;
    p1 = (p - 1.0) / dt;
    vfloat v2 = p1 * p1 + p1 * 2.0 + 1.0;
    vbool cond1 = p < dt;
    vbool cond2 = p > 1.0 - dt;
    return select(cond1, v1, select(cond2, v2, 0.0));
}

vfloat Square(vfloat p, float dt) {
    vfloat v = select(fmod(p, 1.0) < 0.5, 1.0, -1.0);
    v = v + PolyBlep(fmod(p, 1.0), dt);
    v = v - PolyBlep(fmod(p + 0.5, 1.0), dt);
    return v;
}

void Main() {
    vfloat p = time * frequency;
    float dt = frequency / rate;
    vfloat square = Square(p, dt);
    outLeft = square * velocity;
    outRight = square * velocity;
}