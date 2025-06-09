/**
*   VCL Test const aggregate initializer for array
*/

struct Vector3<typename T> {
    T x;
    T y;
    T z;
}

Vector3<float> va = { 1.0, 1.0, -1.0 };
Vector3<float> vb = { -1.0, -1.0, 0.0 };

out float distance;

T Distance<typename T>(Vector3<T> a, Vector3<T> b) {
    return sqrt(pow(b.x - a.x, 2.0) + pow(b.y - a.y, 2.0) + pow(b.z - a.z, 2.0));
}

void Main() {
    distance = Distance(va, vb);
}