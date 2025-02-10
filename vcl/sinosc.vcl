in vfloat time; // In variable a read only and extern. They cannot be initialized but can be binded to via the c++ api to send data.

float frequency = 440.0; // Global variable doesn't need to be in or out. in that case they are not extern but private.
float velocity = 1.0;

out vfloat outLeft; // Out variable are similar to in except you can receive data from them with the c++ api. and they are not read only.
out vfloat outRight;

void Main() {
    vfloat position = time * frequency;
    // Using intrinsic sin function will try to inline corresponding machine instruction.
    vfloat v = sin(position * 2.0 * 3.14159265); // All basic math intrinsic and more are supported. works with scalar and vector type.
    outLeft = v * velocity;
    outRight = v * velocity;
}