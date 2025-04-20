/**
*   Custom type with struct example
*/

struct VectorVector3 {
    vfloat x;
    vfloat y;
    vfloat z;
}

in VectorVector3 a;
in VectorVector3 b;

out vfloat distance;

void Main() {
    distance = sqrt(pow(b.x - a.x, 2.0) + pow(b.y - a.y, 2.0) + pow(b.z - a.z, 2.0));
}