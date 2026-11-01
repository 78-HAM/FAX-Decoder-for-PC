#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include <mutex>

struct SyncSegment {
    int   start_line = 0;
    float skew       = 0.0f;
    float offset     = 0.0f;
};

struct AppSettings {
    std::string language            = "zh-cn"; 
    int   image_width               = 2000;
    float seconds_per_line          = 0.5f;
    float contrast                  = 1.1f;
    bool  is_binary_mode            = false;
    bool  is_light_theme            = false;
    float spectrum_decay_rate       = 0.6f;
    float skew_factor               = 0.0f;
    float global_x_offset           = 0.0f;
    int   selected_audio_device     = -1;
    int   max_lines                 = 2000;
    int   fft_size                  = 8192;
    float cpu_update_interval       = 1.0f; 
};

class SettingsIO {
public:
    static std::string defaultPath();
    static bool load(const std::string& path, AppSettings& out);
    static bool save(const std::string& path, const AppSettings& in);
};

class ImageBuffer {
public:
    ImageBuffer();

    void init(int width, int max_lines);
    void clear();
    void resizeWidth(int new_width);

    void pushLine(const float* line, int width, bool binary_mode);
    void drawLineDirect(const float* line, int row,
                        float skew, float offset, bool binary_mode);
    void reprocessAll(bool binary_mode);
    void rebuildFromRaw(bool binary_mode); // 补全的声明

    const uint8_t* data() const { return pixels_.data(); }
    uint8_t*       data()       { return pixels_.data(); }
    int width()        const { return width_; }
    int maxLines()     const { return max_lines_; }
    int currentLines() const { return current_lines_; }
    bool empty()       const { return current_lines_ == 0; }

    std::vector<uint8_t> snapshot() const;

    void clearSegments();
    void addSegment(const SyncSegment& seg);
    void setSkewOffset(float skew, float offset);
    const std::vector<SyncSegment>& segments() const { return segments_; }
    SyncSegment segmentForLine(int line) const;

    const std::vector<std::vector<float>>& rawLines() const { return raw_lines_; }
    std::vector<std::vector<float>>&       rawLines()       { return raw_lines_; }

    std::mutex& mutex() { return mutex_; }

private:
    void _drawLineNoLock(const float* line, int row,
                         float skew, float offset, bool binary_mode);
    SyncSegment _segmentForLineNoLock(int line) const; // 补全的声明

    int width_ = 2000;
    int max_lines_ = 2000;
    int current_lines_ = 0;

    std::vector<uint8_t> pixels_;
    std::vector<std::vector<float>> raw_lines_;
    std::vector<SyncSegment> segments_;

    mutable std::mutex mutex_;
};