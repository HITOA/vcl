in vfloat values;
in int index;

array<float, 1024> buffer;

void Main() {
    for (int i = 0; i < len(values); ++i) {
        buffer[index + i] = extract(values, i);
    }
}