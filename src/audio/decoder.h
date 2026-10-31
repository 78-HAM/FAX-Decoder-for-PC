#pragma once
#include <atomic>
#include <functional>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <deque>
#include <vector>
#include <string>
#include <utility>

constexpr int TARGET_SAMPLE_RATE = 60000;

class Resampler {
public:
    Resampler(int input_rate, int output_rate);
    void put(const float* samples, int n);
    int  getOutput(float* out, int max_samples);
    void reset();
private:
    float  step_        = 1.0f;
    double input_index_ = 0.0;
    int    read_idx_    = 0; // <--- 新增游标，配合高效重采样
    std::vector<float> buffer_;
};

class PortAudioInput {
public:
    using Callback = std::function<void(const float* samples, int n, int sr)>;

    PortAudioInput();
    ~PortAudioInput();

    bool start(int device_index, int sample_rate, int chunk_size, Callback cb);
    void stop();
    bool running() const { return running_.load(); }

    static std::vector<std::pair<int, std::string>> listInputDevices();

    // 供 PortAudio C 回调调用
    void pushSamples(const float* data, int n);

private:
    void _workerLoop();

    void* stream_ = nullptr;
    std::atomic<bool> running_{false};
    std::atomic<bool> stop_flag_{false};
    Callback cb_;
    int sample_rate_ = 48000;
    std::thread worker_;

    std::mutex buf_mutex_;
    std::condition_variable buf_cv_;
    std::deque<std::vector<float>> buf_queue_;
};