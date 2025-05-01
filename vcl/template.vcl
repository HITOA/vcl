/**
*   struct.vcl example with template
*/

// typename and int are valid template parameters type
struct Vector3<typename T> {
    T x;
    T y;
    T z;
}

in Vector3<vfloat> a;
in Vector3<vfloat> b;

out vfloat distance;

/*T Distance<typename T>(Vector3<T> a, Vector3<T> b) {
    T distance = sqrt(pow(b.x - a.x, 2.0) + pow(b.y - a.y, 2.0) + pow(b.z - a.z, 2.0));
    return distance;
}*/

void Main() {
    distance = sqrt(pow(b.x - a.x, 2.0) + pow(b.y - a.y, 2.0) + pow(b.z - a.z, 2.0));
    float i = distance;
}