#pragma once
#include "imgui.h"
#include <functional>
#include <string>
#include <vector>
#include <cstdint>

// ============================================================
// 公共结构
// ============================================================
struct CalibButton {
    std::string label;
    std::function<void()> cb;
};

struct MainWindowCallbacks {
    std::function<void()> onToggleDecode, onClear;
    std::function<void()> onLoadFont;
    std::function<void()> onPickFile, onStartFileDecode, onSaveImage, onExit;
    std::function<void()> onOpenRecord;
    std::function<void()> onShowAbout, onCheckUpdate;
    std::function<void()> onSelectAudioDevice;
    std::function<void()> onSetSpectrumDecay, onSetImageWidth, onShowContrast;
    std::function<void(const char*)> onSetLanguage;
    std::function<void()> onStartSkewCalib, onStartStartPointCalib;
    std::function<void(float)> onSetRpm;
    std::function<void()> onToggleFullscreen;
    std::function<void(bool)> onSetTheme;
    std::function<void()> onShowManual;
    std::function<void(float*, int)> getSpectrum;
};

// ============================================================
// MainWindow —— 主界面
// ============================================================
class MainWindow {
public:
    void setCallbacks(MainWindowCallbacks cb) { cb_ = std::move(cb); }

    void setStatus(const std::string& s) { status_ = s; }
    void setLanguage(const std::string& lang) { language_ = lang; }
    const std::string& language() const { return language_; }
    void setChineseAvailable(bool b) { chinese_available_ = b; }

    void setDecoding(bool b) { decoding_ = b; }
    void setHasImage(bool b) { has_image_ = b; }
    void setHasFile(bool b)   { has_file_ = b; }
    void setFileName(const std::string& f) { file_name_ = f; }

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
    std::string status_ = "Ready";
    bool decoding_ = false;
    bool has_image_ = false;
    bool has_file_ = false;
    std::string file_name_;

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

// ============================================================
// RecordWindow —— 录音面板
// ============================================================
class RecordWindow {
public:
    struct Callbacks {
        std::function<void()> onToggleRecord;
        std::function<bool()> isRecording;
    };
    void setCallbacks(Callbacks cb) { cb_ = std::move(cb); }

    void show() { visible_ = true; }
    void hide() { visible_ = false; }
    bool visible() const { return visible_; }

    void setStrings(const std::string& title,
                    const std::string& start,
                    const std::string& stop,
                    const std::string& recording,
                    const std::string& idle,
                    const std::string& hint) {
        title_     = title;
        start_     = start;
        stop_      = stop;
        recording_ = recording;
        idle_      = idle;
        hint_      = hint;
    }

    void draw();

private:
    Callbacks cb_;
    bool visible_ = false;
    std::string title_     = "Recording##rec";
    std::string start_     = "Start Recording";
    std::string stop_      = "Stop Recording";
    std::string recording_ = "* Recording...";
    std::string idle_      = "Not recording";
    std::string hint_      = "Temp file stored in system temp dir; save on stop";
};

// ============================================================
// CalibWindow —— 校准状态机
// ============================================================
class CalibWindow {
public:
    enum class Mode {
        NONE,
        SKEW_P1, SKEW_P2,
        START_POINT,
        SPLIT_SELECT,
        MANUAL_MENU,
        MANUAL_START,
        MANUAL_SKEW_P1, MANUAL_SKEW_P2
    };

    struct Hooks {
        std::function<int()> currentLineIndex;
        std::function<void(int)> setTempSplitLine;

        std::function<void(float dskew, float y1_img)> applySkewDelta;
        std::function<void(float x_img)> applyStartPoint;
        std::function<void()> commitManual;

        std::function<void(const char*)> setStatus;
        std::function<void()> openFineTuneSkew;
        std::function<void()> openFineTuneStart;
    };

    void setMainWindow(MainWindow* m) { main_ = m; }
    void setHooks(Hooks h) { hooks_ = std::move(h); }

    void startSkewCalib();
    void startStartPointCalib();
    void showManualMenu();
    void exitCalib();

    Mode mode() const { return mode_; }
    bool active() const { return mode_ != Mode::NONE; }
    bool isManual() const {
        return mode_ == Mode::MANUAL_MENU || mode_ == Mode::MANUAL_START ||
               mode_ == Mode::MANUAL_SKEW_P1 || mode_ == Mode::MANUAL_SKEW_P2;
    }

private:
    void _refreshBar();
    void _skewNext();
    void _applySkew();
    void _applyStartPoint();
    void _finishSplit();
    void _startManualSplit();
    void _startManualStart();
    void _startManualSkew();
    void _manualSkewNext();
    void _applyManualSkew();
    void _commitManual();

    MainWindow* main_ = nullptr;
    Hooks hooks_;
    Mode mode_ = Mode::NONE;
};

// ============================================================
// FineTuneWindow —— 精细调整
// ============================================================
class FineTuneWindow {
public:
    struct Callbacks {
        std::function<void(float skew, float offset)> onApply;
        std::function<void()> onCancel;
    };
    void setCallbacks(Callbacks cb) { cb_ = std::move(cb); }

    void open(bool skew_mode, float base_skew, float base_offset, int image_width);
    void close();
    bool visible() const { return visible_; }

    void setSnapshotTexture(ImTextureID tex, int w, int h) {
        snap_tex_ = tex; snap_w_ = w; snap_h_ = h;
    }

    void draw();

    float currentSkew()   const { return state_skew_; }
    float currentOffset() const { return state_offset_; }
    bool  skewMode()      const { return skew_mode_; }

private:
    Callbacks cb_;
    bool visible_ = false;
    bool skew_mode_ = true;
    int  image_width_ = 2000;

    float base_skew_    = 0.0f;
    float base_offset_  = 0.0f;
    float state_skew_   = 0.0f;
    float state_offset_ = 0.0f;

    float step_         = 0.01f;
    float manual_input_ = 0.0f;

    ImTextureID snap_tex_ = nullptr;
    int snap_w_ = 0, snap_h_ = 0;

    bool dragging_ = false;
    bool is_fullscreen_ = false;
    ImVec2 drag_start_;
    float  drag_skew_   = 0.0f;
    float  drag_offset_ = 0.0f;
    ImVec2 preview_origin_;
    ImVec2 preview_size_;
    float  preview_scale_ = 1.0f;
};