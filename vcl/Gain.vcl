@import "Audio.vcl";

in Audio::AudioBuffer<float32, 2> input;
in float32 gain;

out Audio::AudioBuffer<float32, 2> output;

[EntryPoint]
void Main() {
    output.channels[0] = gain * input.channels[0];
    output.channels[1] = gain * input.channels[1];
}