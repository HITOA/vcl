// Stateful code compiled more than once into one module (like a client emitting one source twice).

float32 state = 0.0;

float32 Step() {
    state += 1.0;
    return state;
}

template<typename T>
T Id(T x) {
    return x;
}

float32 UseTemplates() {
    return Id<float32>(state) + (float32)Id<int32>(1);
}
