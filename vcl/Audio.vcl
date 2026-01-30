template<typename T, uint64 ChannelCount>
export struct AudioBuffer {
    Array<Vec<T>, ChannelCount> channels;
}

export in Vec<float64> Time;
export in uint32 SampleRate;