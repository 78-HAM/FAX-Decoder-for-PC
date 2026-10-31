#include "audio/file.h"

#define DR_WAV_IMPLEMENTATION
#define DR_MP3_IMPLEMENTATION
#define DR_FLAC_IMPLEMENTATION
#include "dr_wav.h"
#include "dr_mp3.h"
#include "dr_flac.h"

#include <algorithm>
#include <cstring>
#include <cctype>

namespace {

std::string g_err;

std::string lowerExt(const std::string& p) {
    auto dot = p.find_last_of('.');
    if (dot == std::string::npos) return "";
    std::string ext = p.substr(dot);
    std::transform(ext.begin(), ext.end(), ext.begin(),
                   [](unsigned char c) { return (char)std::tolower(c); });
    return ext;
}

// 只取第一声道（与 Python 版 data[:, 0] 一致）
void takeFirstChannel(const int16_t* interleaved, size_t frames, int channels,
                      std::vector<int16_t>& out) {
    out.resize(frames);
    if (channels == 1) {
        std::memcpy(out.data(), interleaved, frames * sizeof(int16_t));
        return;
    }
    for (size_t i = 0; i < frames; ++i) {
        out[i] = interleaved[i * channels];
    }
}

bool decodeWav(const std::string& path, AudioFile& out) {
    drwav wav;
    if (!drwav_init_file(&wav, path.c_str(), nullptr)) {
        g_err = "dr_wav init failed";
        return false;
    }
    out.sample_rate = (int)wav.sampleRate;
    out.channels    = (int)wav.channels;
    drwav_uint64 total = wav.totalPCMFrameCount;
    if (total == 0) {
        drwav_uninit(&wav);
        g_err = "empty wav";
        return false;
    }
    std::vector<int16_t> interleaved((size_t)total * out.channels);
    drwav_uint64 got = drwav_read_pcm_frames_s16(&wav, total, interleaved.data());
    takeFirstChannel(interleaved.data(), (size_t)got, out.channels, out.samples);
    drwav_uninit(&wav);
    return true;
}

bool decodeMp3(const std::string& path, AudioFile& out) {
    drmp3 mp3;
    if (!drmp3_init_file(&mp3, path.c_str(), nullptr)) {
        g_err = "dr_mp3 init failed";
        return false;
    }
    out.sample_rate = (int)mp3.sampleRate;
    out.channels    = (int)mp3.channels;
    drmp3_uint64 total = drmp3_get_pcm_frame_count(&mp3);
    if (total == 0) {
        drmp3_uninit(&mp3);
        g_err = "empty mp3";
        return false;
    }
    std::vector<int16_t> interleaved((size_t)total * out.channels);
    drmp3_uint64 got = drmp3_read_pcm_frames_s16(&mp3, total, interleaved.data());
    takeFirstChannel(interleaved.data(), (size_t)got, out.channels, out.samples);
    drmp3_uninit(&mp3);
    return true;
}

bool decodeFlac(const std::string& path, AudioFile& out) {
    drflac* flac = drflac_open_file(path.c_str(), nullptr);
    if (!flac) {
        g_err = "dr_flac init failed";
        return false;
    }
    out.sample_rate = (int)flac->sampleRate;
    out.channels    = (int)flac->channels;
    drflac_uint64 total = flac->totalPCMFrameCount;
    if (total == 0) total = (drflac_uint64)1 << 24;  // 兜底上限
    std::vector<int16_t> interleaved((size_t)total * out.channels);
    drflac_uint64 got = drflac_read_pcm_frames_s16(flac, total, interleaved.data());
    takeFirstChannel(interleaved.data(), (size_t)got, out.channels, out.samples);
    drflac_close(flac);
    return true;
}

} // namespace

std::string AudioFileDecoder::lastError() { return g_err; }

bool AudioFileDecoder::decode(const std::string& path, AudioFile& out) {
    out.samples.clear();
    out.sample_rate = 0;
    out.channels    = 0;
    g_err.clear();

    std::string ext = lowerExt(path);
    if (ext == ".wav" || ext == ".wave") return decodeWav(path, out);
    if (ext == ".mp3")                   return decodeMp3(path, out);
    if (ext == ".flac")                  return decodeFlac(path, out);

    g_err = "unsupported format: " + ext;
    return false;
}