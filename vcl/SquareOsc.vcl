@import "Audio.vcl";

in float32 frequency;
in float32 amplitude;

out Audio::AudioBuffer<float32, 2> output;

Vec<float32> PolyBlep(Vec<float32> p, float32 dt) {
    Vec<float32> p1 = p / dt;
    Vec<float32> v1 = p1 * 2.0 - p1 * p1 - 1.0;
    p1 = (p - 1.0) / dt;
    Vec<float32> v2 = p1 * p1 + p1 * 2.0 + 1.0;
    v2 = select(p > 1.0 - dt, v2, 0.0);
    return select(p < dt, v1, v2);
}

Vec<float32> SquareOsc(Vec<float64> p, uint32 dt) {
    Vec<float32> v = select<float32>(fmod(p, 1.0) < 0.5, 1.0, -1.0);
    v += PolyBlep(fmod(p, 1.0), dt);
    v -= PolyBlep(fmod(p + 0.5, 1.0), dt);
    return v;
}

[EntryPoint]
void Main() {
    Vec<float32> wave = SquareOsc(Audio::Time * frequency, frequency / Audio::SampleRate);
    output.channels[0] = wave * amplitude;
    output.channels[1] = wave * amplitude;
}