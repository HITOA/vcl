@import "Audio.vcl";
@import "Math.vcl";

in float32 frequency;
in float32 amplitude;

out Audio::AudioBuffer<float32, 2> output;

[EntryPoint]
void Main() {
    Vec<float32> wave = (Vec<float32>)sin(2.0 * Math::PI * frequency * Audio::Time);
    output.channels[0] = wave * amplitude;
    output.channels[1] = wave * amplitude;
}