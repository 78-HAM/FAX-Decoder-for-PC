#include "audio/rec_dec.h"
#include <algorithm>
#include <cstring>
#include <cstdlib>
#include <ctime>
#include <chrono>

#ifdef HAVE_PORTAUDIO
#include <portaudio.h>
#endif

// ============================================================
// WAV 写入
// ============================================================
namespace {

void writeU32(std::FILE* f, uint32_t v) { std::fwrite(&v, 4, 1, f); }
void writeU16(std::FILE* f, uint16_t v) { std::fwrite(&v, 2, 1, f); }

void writeWavHeader(std::FILE* f, int sample_rate, int channels, uint32_t data_size) {
    uint32_t byte_rate = (uint32_t)sample_rate * channels * 2;
    uint16_t block_align = (uint16_t)(channels * 2);

    std::fwrite("RIFF", 1, 4, f);
    writeU32(f, 36 + data_size);
    std::fwrite("WAVEfmt ", 1, 8, f);
    writeU32(f, 16);
    writeU16(f, 1);
    writeU16(f, (uint16_t)channels);
    writeU32(f, (uint32_t)sample_rate);
    writeU32(f, byte_rate);
    writeU16(f, block_align);
    writeU16(f, 16);
    std::fwrite("data", 1, 4, f);
    writeU32(f, data_size);
}

std::string makeTempWavPath() {
    const char* tmpdir = std::getenv("TMPDIR");
    if (!tmpdir || !*tmpdir) tmpdir = std::getenv("TEMP");
    if (!tmpdir || !*tmpdir) tmpdir = "/tmp";

    static std::atomic<int> counter{0};
    char buf[512];
    std::snprintf(buf, sizeof(buf),
                  "%s/fax_record_%d_%ld.wav",
                  tmpdir, counter.fetch_add(1),
                  (long)std::time(nullptr));
    return buf;
}

} // namespace

// ============================================================
// PortAudio 引用计数
// ============================================================
#ifdef HAVE_PORTAUDIO
namespace {
std::atomic<int> g_pa_ref{0};
std::mutex g_pa_mutex;

bool paAcquire() {
    std::lock_guard<std::mutex> lk(g_pa_mutex);
    if (g_pa_ref.fetch_add(1) == 0) {
        if (Pa_Initialize() != paNoError) {
            g_pa_ref.fetch_sub(1);
            return false;
        }
    }
    return true;
}

void paRelease() {
    std::lock_guard<std::mutex> lk(g_pa_mutex);
    if (g_pa_ref.fetch_sub(1) == 1) {
        Pa_Terminate();
    }
}

int recCallback(const void* input, void* /*output*/, unsigned long frame_count,
                const PaStreamCallbackTimeInfo* /*ti*/,
                PaStreamCallbackFlags /*flags*/, void* user_data) {
    auto* self = static_cast<Recorder*>(user_data);
    if (!self || !input) return paContinue;
    self->pushSamples(static_cast<const int16_t*>(input), (int)frame_count);
    return paContinue;
}
}
#endif

// ============================================================
// Recorder
// ============================================================
Recorder::Recorder() {}
Recorder::~Recorder() { stop(); }

bool Recorder::_openWav(const std::string& path, int sample_rate, int channels) {
    wav_file_ = std::fopen(path.c_str(), "wb");
    if (!wav_file_) return false;
    writeWavHeader(wav_file_, sample_rate, channels, 0);
    return true;
}

bool Recorder::start(int device_index, int sample_rate, int chunk_size) {
#ifndef HAVE_PORTAUDIO
    (void)device_index; (void)sample_rate; (void)chunk_size;
    return false;
#else
    if (running_.load()) return false;

    int sr = sample_rate > 0 ? sample_rate : 48000;
    int ch = chunk_size > 0 ? chunk_size : 1024;

    temp_path_ = makeTempWavPath();
    if (!_openWav(temp_path_, sr, 1)) {
        temp_path_.clear();
        return false;
    }

    if (!paAcquire()) {
        _finalizeWav();
        temp_path_.clear();
        return false;
    }

    if (device_index < 0) {
        PaDeviceIndex def = Pa_GetDefaultInputDevice();
        if (def == paNoDevice) {
            paRelease();
            _finalizeWav();
            temp_path_.clear();
            return false;
        }
        device_index = def;
    }
    const PaDeviceInfo* info = Pa_GetDeviceInfo(device_index);
    if (!info) {
        paRelease();
        _finalizeWav();
        temp_path_.clear();
        return false;
    }

    PaStreamParameters in_params;
    std::memset(&in_params, 0, sizeof(in_params));
    in_params.device = device_index;
    in_params.channelCount = 1;
    in_params.sampleFormat = paInt16;
    in_params.suggestedLatency = info->defaultLowInputLatency;
    in_params.hostApiSpecificStreamInfo = nullptr;

    PaStream* stream = nullptr;
    PaError err = Pa_OpenStream(&stream, &in_params, nullptr,
                                sr, ch, paClipOff, recCallback, this);
    if (err != paNoError) {
        paRelease();
        _finalizeWav();
        temp_path_.clear();
        return false;
    }

    err = Pa_StartStream(stream);
    if (err != paNoError) {
        Pa_CloseStream(stream);
        paRelease();
        _finalizeWav();
        temp_path_.clear();
        return false;
    }

    stream_ = stream;
    sample_rate_ = sr;
    stop_flag_.store(false);
    running_.store(true);
    worker_ = std::thread(&Recorder::_workerLoop, this);
    return true;
#endif
}

void Recorder::_finalizeWav() {
    if (!wav_file_) return;
    long data_size = std::ftell(wav_file_) - 44;
    if (data_size < 0) data_size = 0;
    std::fseek(wav_file_, 0, SEEK_SET);
    writeWavHeader(wav_file_, sample_rate_, 1, (uint32_t)data_size);
    std::fclose(wav_file_);
    wav_file_ = nullptr;
}

void Recorder::stop() {
    if (!running_.load()) {
        if (wav_file_) _finalizeWav();
        return;
    }

#ifdef HAVE_PORTAUDIO
    if (stream_) {
        PaStream* s = static_cast<PaStream*>(stream_);
        Pa_StopStream(s);
        Pa_CloseStream(s);
        stream_ = nullptr;
        paRelease();
    }
#endif

    stop_flag_.store(true);
    if (worker_.joinable()) worker_.join();
    running_.store(false);

    {
        std::lock_guard<std::mutex> lk(buf_mutex_);
        if (wav_file_ && !buffer_.empty()) {
            std::fwrite(buffer_.data(), sizeof(int16_t), buffer_.size(), wav_file_);
            buffer_.clear();
        }
    }
    _finalizeWav();
}

void Recorder::pushSamples(const int16_t* data, int n) {
    if (!data || n <= 0) return;
    std::lock_guard<std::mutex> lk(buf_mutex_);
    if (buffer_.size() > (size_t)sample_rate_ * 8) return;
    buffer_.insert(buffer_.end(), data, data + n);
}

void Recorder::_workerLoop() {
    while (!stop_flag_.load()) {
        std::vector<int16_t> chunk;
        {
            std::lock_guard<std::mutex> lk(buf_mutex_);
            if (!buffer_.empty()) chunk.swap(buffer_);
        }
        if (!chunk.empty() && wav_file_) {
            std::fwrite(chunk.data(), sizeof(int16_t), chunk.size(), wav_file_);
            std::fflush(wav_file_);
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
}

bool Recorder::saveTo(const std::string& dst) {
    if (temp_path_.empty()) return false;
    std::FILE* in = std::fopen(temp_path_.c_str(), "rb");
    if (!in) return false;
    std::FILE* out = std::fopen(dst.c_str(), "wb");
    if (!out) { std::fclose(in); return false; }
    char buf[8192];
    size_t n;
    while ((n = std::fread(buf, 1, sizeof(buf), in)) > 0) {
        std::fwrite(buf, 1, n, out);
    }
    std::fclose(in);
    std::fclose(out);
    return true;
}