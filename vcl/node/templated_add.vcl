/**
*   Templated add node example
*/

@grog_decl_type Type

in Type inputA;
in Type inputB;

out Type output;

template<typename T>
T Add(T a, T b) {
    return a + b;
}

Audio Add<Audio>(Audio a, Audio b) {
    Audio r = {};
    for (int i = 0; i < a.channelCount; ++i) {
        r.channels[i] = a.channels[i] + b.channels[i];
    }
    return r;
}

void Main() {
    output = Add<Type>(inputA, inputB);
}