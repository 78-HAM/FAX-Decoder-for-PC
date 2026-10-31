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
    std::string language            = "zh_CN";
    int   image_width               = 2000;
    float seconds_per_line          = 0.5f;
    float contrast                  = 1.1f;
    bool  is_binary_mode            = false;
    float spectrum_decay_rate       = 0.6f;
    float skew_factor               = 0.0f;
    float global_x_offset           = 0.0f;
    int   selected_audio_device     = -1;
    int   max_lines                 = 10000;
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
    void rebuildFromRaw(bool binary_mode);

    std::mutex& mutex() { return mutex_; }

private:
    void _drawLineNoLock(const float* line, int row,
                         float skew, float offset, bool binary_mode);

    int width_ = 2000;
    int max_lines_ = 10000;
    int current_lines_ = 0;

    std::vector<uint8_t> pixels_;
    std::vector<std::vector<float>> raw_lines_;
    std::vector<SyncSegment> segments_;

    mutable std::mutex mutex_;
};