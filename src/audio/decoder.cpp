#include "audio/decoder.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <cstdio>

#ifdef HAVE_PORTAUDIO
#include <portaudio.h>
#endif

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
}
#endif

// ============================================================
// Resampler
// ============================================================
Resampler::Resampler(int input_rate, int output_rate) {
    if (input_rate <= 0 || output_rate <= 0) step_ = 1.0f;
    else step_ = (float)input_rate / (float)output_rate;
}

void Resampler::put(const float* samples, int n) {
    if (!samples || n <= 0) return;
    buffer_.insert(buffer_.end(), samples, samples + n);
}

int Resampler::getOutput(float* out, int max_samples) {
    if (!out || max_samples <= 0 || buffer_.size() < 2) return 0;
    int produced = 0;
    while (produced < max_samples) {
        int base = (int)input_index_;
        if (base + 1 >= (int)buffer_.size()) break;
        float frac = (float)(input_index_ - base);
        float s1 = buffer_[base];
        float s2 = buffer_[base + 1];
        out[produced++] = s1 + frac * (s2 - s1);
        input_index_ += step_;
    }
    int consumed = (int)input_index_;
    if (consumed > 0) {
        if (consumed >= (int)buffer_.size()) {
            buffer_.clear();
            input_index_ = 0.0;
        } else {
            buffer_.erase(buffer_.begin(), buffer_.begin() + consumed);
            input_index_ -= consumed;
        }
    }
    return produced;
}

void Resampler::reset() {
    buffer_.clear();
    input_index_ = 0.0;
}

// ============================================================
// PortAudioInput
// ============================================================
#ifdef HAVE_PORTAUDIO
namespace {

int paCallback(const void* input, void* /*output*/, unsigned long frame_count,
               const PaStreamCallbackTimeInfo* /*ti*/,
               PaStreamCallbackFlags /*flags*/, void* user_data) {
    auto* self = static_cast<PortAudioInput*>(user_data);
    if (!self || !input) return paContinue;
    const int16_t* in = static_cast<const int16_t*>(input);
    int n = (int)frame_count;
    static thread_local std::vector<float> tmp;
    tmp.resize(n);
    for (int i = 0; i < n; ++i) tmp[i] = (float)in[i];
    self->pushSamples(tmp.data(), n);
    return paContinue;
}

}
#endif

PortAudioInput::PortAudioInput() {}
PortAudioInput::~PortAudioInput() { stop(); }

bool PortAudioInput::start(int device_index, int sample_rate, int chunk_size, Callback cb) {
#ifndef HAVE_PORTAUDIO
    (void)device_index; (void)sample_rate; (void)chunk_size; (void)cb;
    return false;
#else
    if (running_.load()) return false;

    if (!paAcquire()) return false;

    if (device_index < 0) {
        PaDeviceIndex def = Pa_GetDefaultInputDevice();
        if (def == paNoDevice) { paRelease(); return false; }
        device_index = def;
    }
    const PaDeviceInfo* info = Pa_GetDeviceInfo(device_index);
    if (!info) { paRelease(); return false; }

    PaStreamParameters in_params;
    std::memset(&in_params, 0, sizeof(in_params));
    in_params.device = device_index;
    in_params.channelCount = 1;
    in_params.sampleFormat = paInt16;
    in_params.suggestedLatency = info->defaultLowInputLatency;
    in_params.hostApiSpecificStreamInfo = nullptr;

    int sr = sample_rate > 0 ? sample_rate : 48000;
    int ch = chunk_size > 0 ? chunk_size : 1024;

    PaStream* stream = nullptr;
    PaError err = Pa_OpenStream(&stream, &in_params, nullptr,
                                sr, ch, paClipOff, paCallback, this);
    if (err != paNoError) { paRelease(); return false; }

    err = Pa_StartStream(stream);
    if (err != paNoError) {
        Pa_CloseStream(stream);
        paRelease();
        return false;
    }

    cb_ = std::move(cb);
    sample_rate_ = sr;
    stream_ = stream;
    stop_flag_.store(false);
    running_.store(true);
    worker_ = std::thread(&PortAudioInput::_workerLoop, this);
    return true;
#endif
}

void PortAudioInput::stop() {
    if (!running_.load()) {
#ifdef HAVE_PORTAUDIO
        if (stream_) {
            PaStream* s = static_cast<PaStream*>(stream_);
            Pa_StopStream(s);
            Pa_CloseStream(s);
            stream_ = nullptr;
            paRelease();
        }
#endif
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
    buf_cv_.notify_all();
    if (worker_.joinable()) worker_.join();
    running_.store(false);
    {
        std::lock_guard<std::mutex> lk(buf_mutex_);
        buf_queue_.clear();
    }
}

void PortAudioInput::pushSamples(const float* data, int n) {
    if (!data || n <= 0) return;
    std::lock_guard<std::mutex> lk(buf_mutex_);
    if (buf_queue_.size() > 500) buf_queue_.pop_front(); // 增大到 500 防丢帧
    buf_queue_.emplace_back(data, data + n);
    buf_cv_.notify_one();
}

void PortAudioInput::_workerLoop() {
    while (!stop_flag_.load()) {
        std::vector<float> chunk;
        {
            std::unique_lock<std::mutex> lk(buf_mutex_);
            buf_cv_.wait(lk, [this]() {
                return stop_flag_.load() || !buf_queue_.empty();
            });
            if (stop_flag_.load() && buf_queue_.empty()) break;
            chunk = std::move(buf_queue_.front());
            buf_queue_.pop_front();
        }
        if (cb_ && !chunk.empty()) {
            cb_(chunk.data(), (int)chunk.size(), sample_rate_);
        }
    }
}

std::vector<std::pair<int, std::string>> PortAudioInput::listInputDevices() {
    std::vector<std::pair<int, std::string>> out;
#ifdef HAVE_PORTAUDIO
    if (!paAcquire()) return out;
    int n = Pa_GetDeviceCount();
    for (int i = 0; i < n; ++i) {
        const PaDeviceInfo* info = Pa_GetDeviceInfo(i);
        if (!info) continue;
        if (info->maxInputChannels > 0) {
            out.emplace_back(i, info->name ? info->name : "device");
        }
    }
    paRelease();
#endif
    return out;
}