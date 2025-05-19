/**
*   VCL Test distance with struct & template
*/

struct Vector3<typename T> {
    T x;
    T y;
    T z;
}

in Vector3<float> vecA;
in Vector3<float> vecB;

out float distance;

T Distance<typename T>(Vector3<T> a, Vector3<T> b) {
    return sqrt(pow(b.x - a.x, 2.0) + pow(b.y - a.y, 2.0) + pow(b.z - a.z, 2.0));
}

void Main() {
    distance = Distance(vecA, vecB);
}