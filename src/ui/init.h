#pragma once
#include "imgui.h"
#include <functional>
#include <string>
#include <vector>
#include <cstdint>

struct CalibButton {
    std::string label;
    std::function<void()> cb;
};

struct MainWindowCallbacks {
    std::function<void()> onToggleDecode, onClear;
    std::function<void()> onPickFile, onStartFileDecode, onSaveImage, onExit;
    std::function<void()> onOpenRecord;
    std::function<void()> onShowAbout, onCheckUpdate;
    std::function<void()> onSelectAudioDevice;
    std::function<void()> onSetSpectrumDecay, onSetImageWidth, onShowContrast;
    std::function<void(const char*)> onSetLanguage;
    std::function<void()> onStartSkewCalib, onStartStartPointCalib;
    std::function<void(float)> onSetRpm;
    std::function<void()> onShowManual;
    std::function<void()> onLoadFont;
    std::function<void()> onResetView;
    std::function<void()> onToggleFullscreen;
    std::function<void(bool)> onSetTheme;
    std::function<void(float*, int)> getSpectrum;
};

class MainWindow {
public:
    void setCallbacks(MainWindowCallbacks cb) { cb_ = std::move(cb); }

    void setStatus(const std::string& s) { status_ = s; }
    void setLanguage(const std::string& lang) { language_ = lang; }
    const std::string& language() const { return language_; }
    void setChineseAvailable(bool b) { chinese_available_ = b; }
    void setTheme(bool is_light);

    void setDecoding(bool b) { decoding_ = b; }
    void setHasImage(bool b) { has_image_ = b; }
    void setHasFile(bool b)   { has_file_ = b; }
    void setFileName(const std::string& f) { file_name_ = f; }
    void setLineCount(int count, int max) { current_lines_ = count; max_lines_ = max; }
    void resetView();

    void setImageTexture(ImTextureID tex, int w, int h) {
        image_tex_ = tex; image_w_ = w; image_h_ = h;
    }
    void clearImageTexture() { image_tex_ = nullptr; image_w_ = image_h_ = 0; }

    void setCalibMode(const std::string& m) { calib_mode_ = m; }
    const std::string& calibMode() const { return calib_mode_; }

    void showCalibBar(const std::string& hint, std::vector<CalibButton> btns) {
        calib_hint_ = hint; calib_btns_ = std::move(btns); calib_bar_visible_ = true;
    }
    void hideCalibBar() { calib_bar_visible_ = false; calib_btns_.clear(); }

    void setSpectrumRange(float fmin, float fmax) { fmin_ = fmin; fmax_ = fmax; }

    void draw();

    // 校准点（CalibWindow 直接读写）
    ImVec2 calib_p1{100, 100};
    ImVec2 calib_p2{300, 300};
    float calib_start_x = 500.0f;
    float calib_split_y = 400.0f;

    // 画布视图变换
    float view_scale_     = 1.0f;
    float view_offset_x_  = 0.0f;
    float view_offset_y_  = 0.0f;

    // 坐标换算（CalibWindow 用）
    ImVec2 displayOrigin() const { return image_screen_pos_; }
    ImVec2 displaySize()   const { return image_screen_size_; }
    float  displayScale()  const {
        if (image_screen_size_.x <= 0 || image_w_ <= 0) return 1.0f;
        return image_screen_size_.x / (float)image_w_;
    }
    ImVec2 canvasToImage(ImVec2 p) const {
        float s = displayScale();
        return ImVec2((p.x - image_screen_pos_.x) / s,
                      (p.y - image_screen_pos_.y) / s);
    }

private:
    void drawMenuBar();
    void drawCanvas();
    void drawRightPanel();
    void drawSpectrum();
    void drawCalibBar();
    const char* t(const char* key) const;

    MainWindowCallbacks cb_;
    std::string language_ = "en";
    bool chinese_available_ = false;
    bool is_light_theme_ = false;
    std::string status_ = "Ready";
    bool decoding_ = false;
    bool has_image_ = false;
    bool has_file_ = false;
    std::string file_name_;
    
    int current_lines_ = 0;
    int max_lines_ = 5000;

    ImTextureID image_tex_ = nullptr;
    int image_w_ = 0, image_h_ = 0;

    std::string calib_mode_ = "NONE";
    std::string calib_hint_;
    std::vector<CalibButton> calib_btns_;
    bool calib_bar_visible_ = false;

    float fmin_ = 1500.0f, fmax_ = 2300.0f;

    ImVec2 image_screen_pos_{0, 0};
    ImVec2 image_screen_size_{0, 0};

    bool dragging_ = false;
    bool is_fullscreen_ = false;
    ImVec2 drag_start_{0, 0};
    float drag_off_x_ = 0.0f, drag_off_y_ = 0.0f;
};