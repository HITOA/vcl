/**
*   VCL Test double distance
*/

struct Vector3<typename T> {
    T x;
    T y;
    T z;
}

in Vector3<double> vecA;
in Vector3<double> vecB;

out float distance;

T Distance<typename T>(Vector3<T> a, Vector3<T> b) {
    return sqrt(pow(b.x - a.x, 2.0) + pow(b.y - a.y, 2.0) + pow(b.z - a.z, 2.0));
}

void Main() {
    distance = Distance(vecA, vecB); 
}