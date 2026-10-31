#pragma once
#include "imgui.h"
#include "ui/init.h"
#include "dsp/init.h"
#include "core/image_setting.h"
#include "audio/decoder.h"
#include "audio/file.h"
#include "audio/rec_dec.h"
#include "util/Updater.h"

#include <functional>
#include <string>
#include <memory>
#include <deque>
#include <mutex>
#include <atomic>
#include <thread>
#include <vector>

class I18n {
public:
    I18n();
    void setLanguage(const std::string& lang);
    const std::string& language() const { return language_; }
    const char* t(const char* key) const;
    static const std::vector<std::pair<std::string, std::string>>& languages();
private:
    std::string language_ = "en";
};

class App {
public:
    using TextureUploader = std::function<ImTextureID(const uint8_t*, int, int)>;

    void setTextureUploader(TextureUploader u) { texture_uploader_ = std::move(u); }
    void setChineseAvailable(bool b) { chinese_available_ = b; main_win_.setChineseAvailable(b); }

    void init();
    void shutdown();
    void update();

    bool wantsClose() const { return wants_close_; }
    void requestClose()     { wants_close_ = true; }

    AppSettings&    settings()    { return settings_; }
    I18n&           i18n()        { return i18n_; }
    ImageBuffer&    image()       { return image_; }
    Demodulator&    demodulator() { return demod_; }
    SpectrumDisplay& spectrum()   { return *spectrum_; }

    bool isDecoding()     const { return state_ == State::DECODING; }
    bool isFileDecoding() const { return state_ == State::FILE_DECODING; }
    bool isRecording()    const { return recording_; }
    
    std::string takePendingFontPath() { 
        std::string p = pending_font_path_; 
        pending_font_path_.clear(); 
        return p; 
    }

    void pushRawLine(const float* line_data, int sr);
    void setStatus(const std::string& s);

private:
    enum class State { IDLE, DECODING, FILE_DECODING };
    enum ModalId {
        MODAL_NONE = 0,
        MODAL_SPECTRUM,
        MODAL_WIDTH,
        MODAL_CONTRAST,
        MODAL_ABOUT,
        MODAL_UPDATE,
        MODAL_AUDIO_DEV,
        MODAL_SAVE_RECORD,
        MODAL_MANUAL_SPLIT,
    };

    void _bindCallbacks();
    void _bindCalibHooks();
    void _bindFineTuneHooks();

    void _onStart();
    void _onStop();
    void _onClear();
    void _onPickFile();
    void _onStartFileDecode();
    void _onSaveImage();
    void _onExit();
    void _onOpenRecord();
    void _onShowAbout();
    void _onCheckUpdate();
    void _onSelectAudioDevice();
    void _onSetSpectrumDecay();
    void _onSetImageWidth();
    void _onShowContrast();
    void _onSetLanguage(const char* lang);
    void _onStartSkewCalib();
    void _onStartStartPointCalib();
    void _onSetRpm(float sec);
    void _onShowManual();
    void _onToggleRecord();
    void _onLoadFont();

    void _audioInputCallback(const float* samples, int n, int sr);
    void _fileDecodeWorker(AudioFile file);
    void _consumeLineQueue();
    void _consumeRawForSpectrum();
    void _checkUpdateResult();

    void _updateSpectrumBars(float* out, int n);
    void _markImageDirty() { image_dirty_ = true; }
    void _uploadImageIfDirty();

    void _drawModals();
    void _openModal(ModalId id);
    void _drawModalSpectrum();
    void _drawModalWidth();
    void _drawModalContrast();
    void _drawModalAbout();
    void _drawModalUpdate();
    void _drawModalAudioDev();
    void _drawModalSaveRecord();
    void _drawFileDialog();

    State  state_ = State::IDLE;
    bool   recording_ = false;
    bool   wants_close_ = false;
    bool   has_file_ = false;
    bool   image_dirty_ = true;
    bool   chinese_available_ = false;
    std::string status_;
    std::string current_file_;
    std::string pending_font_path_;

    AppSettings settings_;
    I18n        i18n_;
    ImageBuffer image_;
    Demodulator demod_;
    std::unique_ptr<SpectrumDisplay> spectrum_;

    MainWindow     main_win_;
    RecordWindow   rec_win_;
    CalibWindow    calib_win_;
    FineTuneWindow fine_win_;

    float spectrum_bars_[128] = {};

    bool  is_manual_ = false;
    float temp_skew_ = 0.0f;
    float temp_offset_ = 0.0f;
    int   temp_split_line_ = 0;

    TextureUploader texture_uploader_;
    ImTextureID image_tex_ = nullptr;
    int image_tex_w_ = 0;
    int image_tex_h_ = 0;

    PortAudioInput audio_input_;
    std::unique_ptr<Resampler> input_resampler_;
    std::vector<float> line_acc_;
    int line_acc_count_ = 0;
    int target_line_len_ = 0;

    std::thread file_thread_;
    std::atomic<bool> file_decode_stop_{false};

    std::mutex line_queue_mutex_;
    std::deque<std::vector<float>> line_queue_;

    std::mutex raw_samples_mutex_;
    std::vector<float> raw_samples_;

    Recorder recorder_;
    std::string last_record_path_;

    std::mutex update_result_mutex_;
    UpdateInfo update_result_;
    bool update_result_ready_ = false;

    ModalId modal_id_ = MODAL_NONE;
    bool modal_need_open_ = false;
    char modal_buf_[512] = {};
    int modal_int_ = 0;
    float modal_float_ = 0.0f;
    bool modal_bool_ = false;
    std::string modal_text_;
    std::vector<std::pair<int, std::string>> modal_devices_;
    int modal_selected_device_ = -1;

    bool file_dialog_open_ = false;
};