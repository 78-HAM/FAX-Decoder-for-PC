#pragma once
#include <atomic>
#include <thread>
#include <mutex>
#include <vector>
#include <string>
#include <cstdint>
#include <cstdio>

class Recorder {
public:
    Recorder();
    ~Recorder();

    bool start(int device_index, int sample_rate, int chunk_size);
    void stop();
    bool running() const { return running_.load(); }

    const std::string& tempPath() const { return temp_path_; }
    bool saveTo(const std::string& dst);

    // PortAudio 回调调用
    void pushSamples(const int16_t* data, int n);

private:
    void _workerLoop();
    bool _openWav(const std::string& path, int sample_rate, int channels);
    void _finalizeWav();

    void*  stream_ = nullptr;
    std::atomic<bool> running_{false};
    std::atomic<bool> stop_flag_{false};
    std::thread worker_;
    int sample_rate_ = 48000;

    std::mutex buf_mutex_;
    std::vector<int16_t> buffer_;

    std::string temp_path_;
    std::FILE*  wav_file_ = nullptr;
};