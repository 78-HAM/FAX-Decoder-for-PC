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
// Resampler (优化版)
// ============================================================
Resampler::Resampler(int input_rate, int output_rate) {
    if (input_rate <= 0 || output_rate <= 0) step_ = 1.0f;
    else step_ = (float)input_rate / (float)output_rate;
    input_index_ = 0.0;
    read_idx_ = 0; // 初始化游标
}

void Resampler::put(const float* samples, int n) {
    if (!samples || n <= 0) return;
    buffer_.insert(buffer_.end(), samples, samples + n);
}

int Resampler::getOutput(float* out, int max_samples) {
    if (!out || max_samples <= 0) return 0;
    
    // 实际可读的有效数据大小
    int available = (int)buffer_.size() - read_idx_;
    if (available < 2) return 0;

    int produced = 0;
    // 使用相对偏移进行插值
    while (produced < max_samples) {
        int base = read_idx_ + (int)input_index_;
        if (base + 1 >= (int)buffer_.size()) break;
        
        float frac = (float)(input_index_ - (int)input_index_);
        float s1 = buffer_[base];
        float s2 = buffer_[base + 1];
        out[produced++] = s1 + frac * (s2 - s1);
        input_index_ += step_;
    }

    // 更新已消费的游标
    read_idx_ += (int)input_index_;
    input_index_ -= (int)input_index_;

    // 只有当游标超过缓冲区一半时，才执行真正的 erase，大幅减少内存移动
    if (read_idx_ > (int)buffer_.size() / 2) {
        buffer_.erase(buffer_.begin(), buffer_.begin() + read_idx_);
        read_idx_ = 0;
    }
    
    return produced;
}

void Resampler::reset() {
    buffer_.clear();
    input_index_ = 0.0;
    read_idx_ = 0;
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
#ifdef HAVE_PORTAUDIO
    // 无论 running 状态如何，先确保关闭端口
    if (stream_) {
        PaStream* s = static_cast<PaStream*>(stream_);
        Pa_StopStream(s);
        Pa_CloseStream(s);
        stream_ = nullptr;
        paRelease();
    }
#endif

    if (running_.load()) {
        stop_flag_.store(true);
        buf_cv_.notify_all();
        if (worker_.joinable()) worker_.join();
        running_.store(false);
    }
    
    std::lock_guard<std::mutex> lk(buf_mutex_);
    buf_queue_.clear();
}

void PortAudioInput::pushSamples(const float* data, int n) {
    if (!data || n <= 0) return;
    std::lock_guard<std::mutex> lk(buf_mutex_);
    // 如果队列积压严重，丢弃旧数据保证实时性
    if (buf_queue_.size() > 100) buf_queue_.pop_front(); 
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