in vfloat inputs;

in float delay;

out vfloat outputs;

// Can only take scalar type but will work with matching vector type (vwrite and vread) to insert and read multiple element at once.
ring<float, 1024> buffer; // Circular buffer (ring buffer) of fixed size

void Main() {
    vwrite(buffer, inputs);
    outputs = vread(buffer, delay);
}