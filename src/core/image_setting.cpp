#include "core/image_setting.h"
#include "ini.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <algorithm>
#include <sys/stat.h>
#include <sys/types.h>

#ifdef _WIN32
#  include <direct.h>
#  define MKDIR(p) _mkdir(p)
#else
#  define MKDIR(p) mkdir(p, 0755)
#endif

// ============================================================
// SettingsIO
// ============================================================
static std::string homeDir() {
    const char* h = std::getenv("HOME");
#ifdef _WIN32
    if (!h) h = std::getenv("USERPROFILE");
#endif
    return h ? std::string(h) : std::string(".");
}

std::string SettingsIO::defaultPath() {
    return homeDir() + "/.fax_decoder/setting.ini";
}

static void ensureDir(const std::string& path) {
    std::string acc;
    for (size_t i = 0; i < path.size(); ++i) {
        acc.push_back(path[i]);
        if (path[i] == '/' || path[i] == '\\') {
            if (acc.size() > 1) MKDIR(acc.c_str());
        }
    }
}

namespace {

struct IniCtx {
    AppSettings* s = nullptr;
};

int iniHandler(void* user, const char* section, const char* name,
               const char* value) {
    auto* ctx = static_cast<IniCtx*>(user);
    if (!ctx || !ctx->s || !name || !value) return 1;
    AppSettings& s = *ctx->s;
    std::string sec = section ? section : "";
    std::string n = name;
    std::string v = value;

    if (sec == "general" && n == "language") {
        s.language = v;
    } else if (sec == "image") {
        if      (n == "width")            s.image_width       = std::atoi(v.c_str());
        else if (n == "seconds_per_line") s.seconds_per_line  = (float)std::atof(v.c_str());
        else if (n == "contrast")         s.contrast          = (float)std::atof(v.c_str());
        else if (n == "binary")           s.is_binary_mode    = (std::atoi(v.c_str()) != 0);
        else if (n == "max_lines")        s.max_lines         = std::atoi(v.c_str());
    } else if (sec == "spectrum") {
        if (n == "decay") s.spectrum_decay_rate = (float)std::atof(v.c_str());
    } else if (sec == "calib") {
        if      (n == "skew")   s.skew_factor     = (float)std::atof(v.c_str());
        else if (n == "offset") s.global_x_offset = (float)std::atof(v.c_str());
    } else if (sec == "audio") {
        if (n == "device") s.selected_audio_device = std::atoi(v.c_str());
    }
    return 1;
}

} // namespace

bool SettingsIO::load(const std::string& path, AppSettings& out) {
    IniCtx ctx{&out};
    int r = ini_parse(path.c_str(), iniHandler, &ctx);
    return r >= 0;
}

bool SettingsIO::save(const std::string& path, const AppSettings& in) {
    ensureDir(path);
    FILE* f = std::fopen(path.c_str(), "w");
    if (!f) return false;
    std::fprintf(f, "[general]\nlanguage = %s\n\n", in.language.c_str());
    std::fprintf(f, "[image]\n");
    std::fprintf(f, "width = %d\n", in.image_width);
    std::fprintf(f, "seconds_per_line = %.6f\n", in.seconds_per_line);
    std::fprintf(f, "contrast = %.4f\n", in.contrast);
    std::fprintf(f, "binary = %d\n", in.is_binary_mode ? 1 : 0);
    std::fprintf(f, "max_lines = %d\n\n", in.max_lines);
    std::fprintf(f, "[spectrum]\ndecay = %.4f\n\n", in.spectrum_decay_rate);
    std::fprintf(f, "[calib]\n");
    std::fprintf(f, "skew = %.6f\n", in.skew_factor);
    std::fprintf(f, "offset = %.4f\n\n", in.global_x_offset);
    std::fprintf(f, "[audio]\ndevice = %d\n", in.selected_audio_device);
    std::fclose(f);
    return true;
}

// ============================================================
// ImageBuffer
// ============================================================
ImageBuffer::ImageBuffer() { init(2000, 10000); }

void ImageBuffer::init(int width, int max_lines) {
    std::lock_guard<std::mutex> lk(mutex_);
    width_        = std::max(1, width);
    max_lines_    = std::max(1, max_lines);
    current_lines_ = 0;
    pixels_.assign((size_t)width_ * max_lines_, 0);
    raw_lines_.clear();
    segments_.clear();
    segments_.push_back({0, 0.0f, 0.0f});
}

void ImageBuffer::clear() {
    std::lock_guard<std::mutex> lk(mutex_);
    std::fill(pixels_.begin(), pixels_.end(), (uint8_t)0);
    raw_lines_.clear();
    current_lines_ = 0;
    segments_.clear();
    segments_.push_back({0, 0.0f, 0.0f});
}

void ImageBuffer::resizeWidth(int new_width) {
    std::lock_guard<std::mutex> lk(mutex_);
    width_ = std::max(1, new_width);
    pixels_.assign((size_t)width_ * max_lines_, 0);
    raw_lines_.clear();
    current_lines_ = 0;
}

void ImageBuffer::_drawLineNoLock(const float* line, int row,
                                  float skew, float offset, bool binary_mode) {
    if (!line || row < 0 || row >= max_lines_) return;
    const float W = (float)width_;
    const float total = (float)row * skew + offset;
    const float eff = std::fmod(std::fmod(total, W) + W, W);

    uint8_t* dst = pixels_.data() + (size_t)row * width_;
    for (int x = 0; x < width_; ++x) {
        float src = (float)x - eff;
        src = std::fmod(std::fmod(src, W) + W, W);
        int x1 = (int)src;
        int x2 = (x1 + 1) % width_;
        float frac = src - (float)x1;
        float v = line[x1] * (1.0f - frac) + line[x2] * frac;
        if (v < 0.0f) v = 0.0f;
        if (v > 1.0f) v = 1.0f;
        dst[x] = binary_mode ? ((v > 0.5f) ? 255 : 0)
                             : (uint8_t)(v * 255.0f);
    }
}

void ImageBuffer::drawLineDirect(const float* line, int row,
                                 float skew, float offset, bool binary_mode) {
    std::lock_guard<std::mutex> lk(mutex_);
    _drawLineNoLock(line, row, skew, offset, binary_mode);
}

void ImageBuffer::pushLine(const float* line, int width, bool binary_mode) {
    if (!line || width != width_) return;
    std::lock_guard<std::mutex> lk(mutex_);
    int row = current_lines_;
    if (row >= max_lines_) return;
    SyncSegment seg = segmentForLine(row);
    _drawLineNoLock(line, row, seg.skew, seg.offset, binary_mode);
    raw_lines_.emplace_back(line, line + width);
    current_lines_ = row + 1;
}

void ImageBuffer::reprocessAll(bool binary_mode) {
    std::lock_guard<std::mutex> lk(mutex_);
    for (int row = 0; row < current_lines_; ++row) {
        SyncSegment seg = segmentForLine(row);
        _drawLineNoLock(raw_lines_[row].data(), row, seg.skew, seg.offset, binary_mode);
    }
}

void ImageBuffer::rebuildFromRaw(bool binary_mode) {
    reprocessAll(binary_mode);
}

SyncSegment ImageBuffer::segmentForLine(int line) const {
    SyncSegment result{0, 0.0f, 0.0f};
    for (auto& s : segments_) {
        if (line >= s.start_line) result = s;
        else break;
    }
    return result;
}

void ImageBuffer::clearSegments() {
    std::lock_guard<std::mutex> lk(mutex_);
    segments_.clear();
    segments_.push_back({0, 0.0f, 0.0f});
}

void ImageBuffer::addSegment(const SyncSegment& seg) {
    std::lock_guard<std::mutex> lk(mutex_);
    // 保持按 start_line 升序
    segments_.erase(std::remove_if(segments_.begin(), segments_.end(),
                                   [&](const SyncSegment& s) {
                                       return s.start_line >= seg.start_line;
                                   }),
                    segments_.end());
    segments_.push_back(seg);
    std::sort(segments_.begin(), segments_.end(),
              [](const SyncSegment& a, const SyncSegment& b) {
                  return a.start_line < b.start_line;
              });
    if (segments_.empty() || segments_.front().start_line != 0) {
        segments_.insert(segments_.begin(), {0, 0.0f, 0.0f});
    }
}

void ImageBuffer::setSkewOffset(float skew, float offset) {
    std::lock_guard<std::mutex> lk(mutex_);
    segments_.clear();
    segments_.push_back({0, skew, offset});
}

std::vector<uint8_t> ImageBuffer::snapshot() const {
    std::lock_guard<std::mutex> lk(mutex_);
    if (current_lines_ == 0) return {};
    return std::vector<uint8_t>(pixels_.begin(),
                                pixels_.begin() + (size_t)current_lines_ * width_);
}