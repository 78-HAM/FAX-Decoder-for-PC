#pragma once
#include <cstdint>
#include <string>
#include <vector>

struct AudioFile {
    std::vector<int16_t> samples;
    int sample_rate = 0;
    int channels    = 0;
    bool empty() const { return samples.empty(); }
};

class AudioFileDecoder {
public:
    static bool decode(const std::string& path, AudioFile& out);
    static std::string lastError();
};