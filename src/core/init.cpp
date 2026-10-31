#include "core/init.h"
#include "util/ImageIO.h"
#ifdef HAVE_IMGUI_FILE_DIALOG
#include "ImGuiFileDialog.h"
#endif

#include <unordered_map>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <chrono>

// ============================================================
// I18n
// ============================================================
namespace {
struct LangPair { std::string code, name; };
const std::vector<LangPair>& langList() {
    static const std::vector<LangPair> L = {
        {"zh_CN", "简体中文"}, {"en", "English"}, {"ru", "Русский"},
        {"ja", "日本語"}, {"zh_TW", "繁體中文"},
    };
    return L;
}

using TR5 = std::array<const char*, 5>;
const std::unordered_map<std::string, TR5>& trTable() {
    static const std::unordered_map<std::string, TR5> m = {
        {"status.ready",                 {"Ready", "就绪", "Готов", "準備完了", "就緒"}},
        {"status.decoding",              {"Decoding...", "正在解码…", "Декодирование…", "デコード中…", "解碼中…"}},
        {"status.stopped",               {"Stopped", "已停止", "Остановлено", "停止しました", "已停止"}},
        {"status.file_decoding",         {"File decoding...", "文件解码中…", "Декодирование файла…", "ファイルデコード中…", "檔案解碼中…"}},
        {"status.file_done",             {"File decoding done", "文件解码完成", "Декодирование завершено", "ファイルデコード完了", "檔案解碼完成"}},
        {"status.file_failed",           {"Decode failed", "解码失败", "Ошибка декодирования", "デコード失敗", "解碼失敗"}},
        {"status.cleared",               {"Image cleared", "已清空图像", "Изображение очищено", "画像をクリアしました", "已清空圖片"}},
        {"status.image_saved",           {"Image saved", "图像已保存", "Изображение сохранено", "画像を保存しました", "圖片已儲存"}},
        {"status.save_cancelled",        {"Save cancelled", "已取消保存", "Сохранение отменено", "保存をキャンセル", "已取消儲存"}},
        {"status.recording",             {"Recording...", "正在录制…", "Запись…", "録音中…", "正在錄製…"}},
        {"status.recording_done",        {"Recording stopped", "录音结束", "Запись завершена", "録音終了", "錄音結束"}},
        {"status.recording_saved",       {"Recording saved", "录音已保存", "Запись сохранена", "録音を保存しました", "錄音已儲存"}},
        {"status.audio_device_reset",    {"Audio device reset", "音频设备已重置", "Устройство сброшено", "デバイスをリセット", "音訊裝置已重設"}},
        {"status.audio_device_selected", {"Device selected", "已选设备", "Устройство выбрано", "デバイスを選択", "已選裝置"}},
        {"status.per_line",              {"Per line", "每行时长", "Строка", "1行あたり", "每行時長"}},
        {"status.spectrum_decay",        {"Spectrum decay", "频谱衰减", "Затухание", "スペクトル減衰", "頻譜衰減"}},
        {"status.loading_file",          {"File selected", "已选文件", "Файл выбран", "ファイル選択", "已選檔案"}},
        {"status.exited_calib",          {"Calibration exited", "已退出校准", "Калибровка завершена", "キャリブレーション終了", "已離開校正"}},
        {"status.manual_applied",        {"Manual adjustment applied", "手动调整已应用", "Ручная настройка применена", "手動調整適用", "手動調整已套用"}},
        {"status.split_line",            {"Split line", "分割线", "Линия раздела", "分割線", "分割線"}},
        {"status.no_image",              {"No image data", "尚无图像数据", "Нет данных", "画像データなし", "尚無圖片資料"}},
        {"status.no_file",               {"No file selected", "未选择文件", "Файл не выбран", "ファイル未選択", "未選擇檔案"}},
        {"status.update_checking",       {"Checking updates...", "正在检查更新…", "Проверка обновлений…", "更新を確認中…", "正在檢查更新…"}},
        {"status.update_latest",         {"Already latest version", "已是最新版本", "Уже последняя версия", "最新バージョンです", "已是最新版本"}},
        {"status.update_found",          {"New version found", "发现新版本", "Найдена новая версия", "新しいバージョン", "發現新版本"}},
        {"status.update_failed",         {"Update check failed", "检查更新失败", "Проверка не удалась", "更新確認に失敗", "檢查更新失敗"}},
        {"status.record_open_failed",    {"Recording device unavailable", "录音设备不可用", "Устройство недоступно", "録音デバイス不可", "錄音裝置不可用"}},
        {"status.audio_open_failed",     {"Audio input unavailable", "音频输入不可用", "Аудио недоступно", "音声入力不可", "音訊輸入不可用"}},
        {"status.manual_need_stop",      {"Stop decoding first", "请先停止解码", "Сначала остановите", "先に停止してください", "請先停止解碼"}},
        {"status.exit_calib_first",      {"Exit calibration first", "请先退出校准", "Сначала выйдите из калибровки", "先にキャリブレーション終了", "請先離開校正"}},
        {"status.file_running",          {"File decoding running", "文件解码进行中", "Декодирование идёт", "デコード中", "檔案解碼進行中"}},
        {"status.already_decoding",      {"Decoding already in progress", "解码已在进行中", "Уже идёт", "デコード中です", "解碼已在進行中"}},
        {"status.pick_file",             {"Choose a file from dialog", "请在对话框中选择文件", "Выберите файл", "ファイルを選択", "請在對話框中選擇檔案"}},
        {"status.device_selected",       {"Device: ", "设备：", "Устройство: ", "デバイス：", "裝置："}},
    };
    return m;
}

int langIndex(const std::string& code) {
    if (code == "en")    return 0;
    if (code == "zh_CN") return 1;
    if (code == "ru")    return 2;
    if (code == "ja")    return 3;
    if (code == "zh_TW") return 4;
    return 0;
}
} // namespace

I18n::I18n() {}
void I18n::setLanguage(const std::string& lang) { language_ = lang; }
const char* I18n::t(const char* key) const {
    auto& m = trTable();
    auto it = m.find(key);
    if (it == m.end()) return key;
    return it->second[langIndex(language_)];
}
const std::vector<std::pair<std::string, std::string>>& I18n::languages() {
    static std::vector<std::pair<std::string, std::string>> L;
    if (L.empty()) for (auto& p : langList()) L.push_back({p.code, p.name});
    return L;
}

// ============================================================
// App - 初始化
// ============================================================
void App::init() {
    std::string cfg_path = SettingsIO::defaultPath();
    SettingsIO::load(cfg_path, settings_);

    i18n_.setLanguage(settings_.language);
    main_win_.setLanguage(settings_.language);
    main_win_.setChineseAvailable(chinese_available_);

    image_.init(settings_.image_width, settings_.max_lines);
    demod_.setConfig({1500.0f, 2300.0f, 128, settings_.contrast});
    spectrum_.reset(new SpectrumDisplay(2048));

    rec_win_.setStrings("Recording##rec",
                        "Start Recording", "Stop Recording",
                        "* Recording...", "Not recording",
                        "Temp file stored in system temp dir; save on stop");

    _bindCallbacks();
    _bindCalibHooks();
    _bindFineTuneHooks();

    setStatus(i18n_.t("status.ready"));
    main_win_.setHasFile(false);
    main_win_.setHasImage(false);
    image_dirty_ = false;
}

void App::shutdown() {
    file_decode_stop_.store(true);
    audio_input_.stop();
    if (file_thread_.joinable()) file_thread_.join();
    recorder_.stop();
    image_tex_ = nullptr;
}

void App::setStatus(const std::string& s) {
    status_ = s;
    main_win_.setStatus(s);
}

// ============================================================
// App - 每帧主循环
// ============================================================
void App::update() {
    // 音频输入 / 文件解码产生的行，在主线程消费
    _consumeLineQueue();
    _consumeRawForSpectrum();
    _checkUpdateResult();

    spectrum_->updateDecay(settings_.spectrum_decay_rate);
    _uploadImageIfDirty();

    main_win_.draw();
    fine_win_.draw();
    rec_win_.draw();

    // 模态框 & 文件对话框必须在窗口之后
    _drawFileDialog();
    _drawModals();
}

// ============================================================
// 音频输入回调（worker 线程）
// ============================================================
void App::_audioInputCallback(const float* samples, int n, int sr) {
    if (!samples || n <= 0) return;

    // 频谱用原始样本
    {
        std::lock_guard<std::mutex> lk(raw_samples_mutex_);
        raw_samples_.insert(raw_samples_.end(), samples, samples + n);
        const size_t kMax = 65536;
        if (raw_samples_.size() > kMax) {
            raw_samples_.erase(raw_samples_.begin(),
                               raw_samples_.begin() + (raw_samples_.size() - kMax));
        }
    }

    // 重采样
    if (!input_resampler_) return;
    input_resampler_->put(samples, n);

    std::vector<float> out_buf(n * 2 + 64);
    int got = input_resampler_->getOutput(out_buf.data(), (int)out_buf.size());
    if (got <= 0) return;

    int pos = 0;
    while (pos < got) {
        int space = target_line_len_ - line_acc_count_;
        int take = std::min(space, got - pos);
        std::memcpy(&line_acc_[line_acc_count_], &out_buf[pos], take * sizeof(float));
        line_acc_count_ += take;
        pos += take;

        if (line_acc_count_ >= target_line_len_) {
            std::lock_guard<std::mutex> lk(line_queue_mutex_);
            if (line_queue_.size() < 64) {
                line_queue_.emplace_back(line_acc_.begin(), line_acc_.end());
            }
            line_acc_count_ = 0;
        }
    }
}

// ============================================================
// 文件解码线程
// ============================================================
void App::_fileDecodeWorker(AudioFile file) {
    Resampler rs(file.sample_rate, TARGET_SAMPLE_RATE);
    int target_len = (int)(TARGET_SAMPLE_RATE * settings_.seconds_per_line);

    std::vector<float> fl(file.samples.size());
    for (size_t i = 0; i < file.samples.size(); ++i) fl[i] = (float)file.samples[i];
    rs.put(fl.data(), (int)fl.size());

    std::vector<float> line_acc(target_len);
    int line_count = 0;
    std::vector<float> out_buf(8192);

    auto flush_line = [&]() {
        std::lock_guard<std::mutex> lk(line_queue_mutex_);
        while (line_queue_.size() >= 32 && !file_decode_stop_.load()) {
            // 队列满，暂停（睡一小会儿）
            // 这里用简单自旋+睡眠，主线程会尽快消费
        }
        line_queue_.emplace_back(line_acc.begin(), line_acc.end());
    };

    while (!file_decode_stop_.load()) {
        int got = rs.getOutput(out_buf.data(), (int)out_buf.size());
        if (got <= 0) break;

        int pos = 0;
        while (pos < got && !file_decode_stop_.load()) {
            int space = target_len - line_count;
            int take = std::min(space, got - pos);
            std::memcpy(&line_acc[line_count], &out_buf[pos], take * sizeof(float));
            line_count += take;
            pos += take;

            if (line_count >= target_len) {
                // 队列满时等主线程消费
                int wait_count = 0;
                while (!file_decode_stop_.load()) {
                    {
                        std::lock_guard<std::mutex> lk(line_queue_mutex_);
                        if (line_queue_.size() < 32) break;
                    }
                    std::this_thread::sleep_for(std::chrono::milliseconds(5));
                    if (++wait_count > 2000) break;  // 10 秒超时
                }
                if (file_decode_stop_.load()) break;

                std::lock_guard<std::mutex> lk(line_queue_mutex_);
                line_queue_.emplace_back(line_acc.begin(), line_acc.end());
                line_count = 0;
            }
        }
    }

    // 处理剩余的尾部
    if (!file_decode_stop_.load() && line_count > 0) {
        std::lock_guard<std::mutex> lk(line_queue_mutex_);
        line_queue_.emplace_back(line_acc.begin(), line_acc.end());
    }
}

// ============================================================
// 主线程消费队列
// ============================================================
void App::_consumeLineQueue() {
    const int kMaxPerFrame = 20;
    int count = 0;
    while (count < kMaxPerFrame) {
        std::vector<float> line;
        {
            std::lock_guard<std::mutex> lk(line_queue_mutex_);
            if (line_queue_.empty()) break;
            line = std::move(line_queue_.front());
            line_queue_.pop_front();
        }
        pushRawLine(line.data(), TARGET_SAMPLE_RATE);
        ++count;
    }
    if (count > 0) image_dirty_ = true;

    // 文件解码结束检测
    if (state_ == State::FILE_DECODING) {
        bool finished = false;
        if (file_thread_.joinable()) {
            // 检查队列空 + 线程结束（用 minimal 状态推断）
            std::lock_guard<std::mutex> lk(line_queue_mutex_);
            if (line_queue_.empty()) {
                finished = true;  // 简单判断：队列空 + 状态仍是 FILE_DECODING 视为可能的结束
            }
        }
        // 不在这里 join，由 _onStop 或结束后手动处理
        (void)finished;
    }
}

void App::_consumeRawForSpectrum() {
    std::vector<float> local;
    {
        std::lock_guard<std::mutex> lk(raw_samples_mutex_);
        if (raw_samples_.empty()) return;
        local.swap(raw_samples_);
    }
    spectrum_->pushSamples(local.data(), (int)local.size());
}

void App::_checkUpdateResult() {
    std::lock_guard<std::mutex> lk(update_result_mutex_);
    if (!update_result_ready_) return;
    update_result_ready_ = false;
    const UpdateInfo& info = update_result_;
    if (!info.success) {
        setStatus(i18n_.t("status.update_failed"));
    } else if (info.has_update) {
        modal_text_ = std::string(i18n_.t("status.update_found")) + " " +
                      info.latest_version + "\n" +
                      info.latest_url + "\n\n" +
                      i18n_.t("status.ready") + " " + info.current_version;
        _openModal(MODAL_UPDATE);
    } else {
        setStatus(i18n_.t("status.update_latest"));
    }
}

// ============================================================
// 推一行原始音频 → 解调 → 写入 image
// ============================================================
void App::pushRawLine(const float* line_data, int sr) {
    if (!line_data) return;
    std::vector<float> demod_out((size_t)settings_.image_width);
    demod_.demodulateLine(line_data, sr, 48000,
                          settings_.image_width, demod_out.data());

    {
        if (image_.currentLines() >= settings_.max_lines) return;
        image_.pushLine(demod_out.data(), settings_.image_width,
                        settings_.is_binary_mode);
    }
    image_dirty_ = true;
    main_win_.setHasImage(true);
}

// ============================================================
// 频谱条
// ============================================================
void App::_updateSpectrumBars(float* out, int n) {
    DspUtil::resampleSpectrumToBars(
        spectrum_->displayData(), spectrum_->nBins(),
        48000, spectrum_->fftSize(), 4000.0f,
        out, n);
}

void App::_uploadImageIfDirty() {
    if (!image_dirty_ || !texture_uploader_) return;
    int lines = image_.currentLines();
    if (lines <= 0) {
        image_dirty_ = false;
        main_win_.clearImageTexture();
        return;
    }
    auto snap = image_.snapshot();
    if (snap.empty()) { image_dirty_ = false; return; }
    image_tex_ = texture_uploader_(snap.data(), image_.width(), lines);
    image_tex_w_ = image_.width();
    image_tex_h_ = lines;
    main_win_.setImageTexture(image_tex_, image_tex_w_, image_tex_h_);
    image_dirty_ = false;
}

// ============================================================
// 回调绑定
// ============================================================
void App::_bindCallbacks() {
    MainWindowCallbacks cb;
    cb.onStart            = [this]() { _onStart(); };
    cb.onStop             = [this]() { _onStop(); };
    cb.onClear            = [this]() { _onClear(); };
    cb.onExit             = [this]() { _onExit(); };
    cb.onOpenRecord       = [this]() { _onOpenRecord(); };
    cb.onPickFile         = [this]() { _onPickFile(); };
    cb.onStartFileDecode  = [this]() { _onStartFileDecode(); };
    cb.onSaveImage        = [this]() { _onSaveImage(); };
    cb.onShowAbout        = [this]() { _onShowAbout(); };
    cb.onCheckUpdate      = [this]() { _onCheckUpdate(); };
    cb.onResetAudioDevice = [this]() { _onResetAudioDevice(); };
    cb.onSelectAudioDevice = [this]() { _onSelectAudioDevice(); };
    cb.onSetSpectrumDecay = [this]() { _onSetSpectrumDecay(); };
    cb.onSetImageWidth    = [this]() { _onSetImageWidth(); };
    cb.onShowContrast     = [this]() { _onShowContrast(); };
    cb.onSetLanguage      = [this](const char* l) { _onSetLanguage(l); };
    cb.onStartSkewCalib        = [this]() { _onStartSkewCalib(); };
    cb.onStartStartPointCalib  = [this]() { _onStartStartPointCalib(); };
    cb.onSetRpm           = [this](float s) { _onSetRpm(s); };
    cb.onShowManual       = [this]() { _onShowManual(); };
    cb.getSpectrum        = [this](float* out, int n) { _updateSpectrumBars(out, n); };

    main_win_.setCallbacks(cb);

    RecordWindow::Callbacks rcb;
    rcb.isRecording    = [this]() { return recording_; };
    rcb.onToggleRecord = [this]() { _onToggleRecord(); };
    rec_win_.setCallbacks(rcb);
}

void App::_bindCalibHooks() {
    calib_win_.setMainWindow(&main_win_);

    CalibWindow::Hooks hk;
    hk.currentLineIndex = [this]() { return image_.currentLines(); };
    hk.setTempSplitLine = [this](int line) { temp_split_line_ = line; };

    hk.applySkewDelta = [this](float dskew, float y1) {
        float& sk  = is_manual_ ? temp_skew_   : settings_.skew_factor;
        float& off = is_manual_ ? temp_offset_ : settings_.global_x_offset;
        float old = sk;
        sk += dskew;
        off = std::fmod(off + y1 * (old - sk), (float)settings_.image_width);
        if (off < 0) off += settings_.image_width;

        if (!is_manual_) {
            image_.setSkewOffset(settings_.skew_factor, settings_.global_x_offset);
        }
        image_.reprocessAll(settings_.is_binary_mode);
        image_dirty_ = true;
    };

    hk.applyStartPoint = [this](float x_img) {
        float& off = is_manual_ ? temp_offset_ : settings_.global_x_offset;
        off = std::fmod(off - x_img, (float)settings_.image_width);
        if (off < 0) off += settings_.image_width;

        if (!is_manual_) {
            image_.setSkewOffset(settings_.skew_factor, settings_.global_x_offset);
        }
        image_.reprocessAll(settings_.is_binary_mode);
        image_dirty_ = true;
    };

    hk.commitManual = [this]() {
        image_.addSegment({temp_split_line_, temp_skew_, temp_offset_});
        image_.reprocessAll(settings_.is_binary_mode);
        is_manual_ = false;
        temp_skew_ = 0.0f;
        temp_offset_ = 0.0f;
        temp_split_line_ = 0;
        image_dirty_ = true;
        setStatus(i18n_.t("status.manual_applied"));
    };

    hk.setStatus = [this](const char* s) { setStatus(s); };

    hk.openFineTuneSkew = [this]() {
        if (image_.currentLines() <= 0) return;
        if (image_tex_) {
            fine_win_.setSnapshotTexture(image_tex_,
                                         image_.width(), image_.currentLines());
        }
        float base_s = is_manual_ ? temp_skew_ : settings_.skew_factor;
        float base_o = is_manual_ ? temp_offset_ : settings_.global_x_offset;
        fine_win_.open(true, base_s, base_o, settings_.image_width);
    };
    hk.openFineTuneStart = [this]() {
        if (image_.currentLines() <= 0) return;
        if (image_tex_) {
            fine_win_.setSnapshotTexture(image_tex_,
                                         image_.width(), image_.currentLines());
        }
        float base_s = is_manual_ ? temp_skew_ : settings_.skew_factor;
        float base_o = is_manual_ ? temp_offset_ : settings_.global_x_offset;
        fine_win_.open(false, base_s, base_o, settings_.image_width);
    };

    calib_win_.setHooks(hk);
}

void App::_bindFineTuneHooks() {
    FineTuneWindow::Callbacks fcb;
    fcb.onApply = [this](float skew, float offset) {
        offset = std::fmod(offset, (float)settings_.image_width);
        if (offset < 0) offset += settings_.image_width;

        if (is_manual_) {
            temp_skew_ = skew;
            temp_offset_ = offset;
        } else {
            settings_.skew_factor = skew;
            settings_.global_x_offset = offset;
            image_.setSkewOffset(skew, offset);
        }
        image_.reprocessAll(settings_.is_binary_mode);
        image_dirty_ = true;
        setStatus(i18n_.t("status.manual_applied"));
    };
    fcb.onCancel = [this]() { setStatus(i18n_.t("status.stopped")); };
    fine_win_.setCallbacks(fcb);
}

// ============================================================
// 启动 / 停止
// ============================================================
void App::_onStart() {
    if (state_ == State::DECODING) {
        setStatus(i18n_.t("status.already_decoding"));
        return;
    }
    if (state_ == State::FILE_DECODING) {
        setStatus(i18n_.t("status.file_running"));
        return;
    }

    input_resampler_.reset(new Resampler(48000, TARGET_SAMPLE_RATE));
    target_line_len_ = (int)(TARGET_SAMPLE_RATE * settings_.seconds_per_line);
    line_acc_.assign(target_line_len_, 0.0f);
    line_acc_count_ = 0;

    bool ok = audio_input_.start(
        settings_.selected_audio_device, 48000, 1024,
        [this](const float* s, int n, int sr) { _audioInputCallback(s, n, sr); });

    if (!ok) {
        setStatus(i18n_.t("status.audio_open_failed"));
        return;
    }

    state_ = State::DECODING;
    main_win_.setDecoding(true);
    setStatus(i18n_.t("status.decoding"));
}

void App::_onStop() {
    if (state_ == State::DECODING) {
        audio_input_.stop();
        state_ = State::IDLE;
        main_win_.setDecoding(false);
        setStatus(i18n_.t("status.stopped"));
    } else if (state_ == State::FILE_DECODING) {
        file_decode_stop_.store(true);
        if (file_thread_.joinable()) file_thread_.join();
        state_ = State::IDLE;
        main_win_.setDecoding(false);
        setStatus(i18n_.t("status.stopped"));
    }
}

void App::_onClear() {
    if (calib_win_.active()) calib_win_.exitCalib();
    {
        std::lock_guard<std::mutex> lk(line_queue_mutex_);
        line_queue_.clear();
    }
    {
        std::lock_guard<std::mutex> lk(raw_samples_mutex_);
        raw_samples_.clear();
    }
    image_.clear();
    spectrum_->reset();
    main_win_.clearImageTexture();
    main_win_.setHasImage(false);
    image_tex_ = nullptr;
    image_tex_w_ = image_tex_h_ = 0;
    setStatus(i18n_.t("status.cleared"));
}

// ============================================================
// 文件选择 / 文件解码
// ============================================================
void App::_onPickFile() {
    file_dialog_open_ = true;
#ifdef HAVE_IMGUI_FILE_DIALOG
    IGFD::FileDialog::Instance()->OpenDialog(
        "ChooseAudio", "Select Audio File",
        ".wav,.mp3,.flac,.ogg,.aiff,.au");
#else
    // 兜底：进入 modal，让用户输入路径
    modal_buf_[0] = '\0';
    _openModal(MODAL_SAVE_RECORD);  // 复用文本输入 modal（name 会显示）
    setStatus(i18n_.t("status.pick_file"));
#endif
}

void App::_onStartFileDecode() {
    if (!has_file_ || current_file_.empty()) {
        setStatus(i18n_.t("status.no_file"));
        return;
    }
    if (state_ != State::IDLE) {
        setStatus(i18n_.t("status.file_running"));
        return;
    }

    AudioFile file;
    if (!AudioFileDecoder::decode(current_file_, file) || file.empty()) {
        setStatus(std::string(i18n_.t("status.file_failed")) +
                  ": " + AudioFileDecoder::lastError());
        return;
    }

    // 清空旧图
    image_.clear();
    {
        std::lock_guard<std::mutex> lk(line_queue_mutex_);
        line_queue_.clear();
    }
    {
        std::lock_guard<std::mutex> lk(raw_samples_mutex_);
        raw_samples_.clear();
    }
    spectrum_->reset();
    image_tex_ = nullptr;
    image_tex_w_ = image_tex_h_ = 0;
    main_win_.clearImageTexture();

    file_decode_stop_.store(false);
    state_ = State::FILE_DECODING;
    main_win_.setDecoding(true);
    setStatus(i18n_.t("status.file_decoding"));

    if (file_thread_.joinable()) file_thread_.join();
    file_thread_ = std::thread(&App::_fileDecodeWorker, this, std::move(file));
}

// ============================================================
// 保存图像
// ============================================================
void App::_onSaveImage() {
    if (image_.empty()) {
        setStatus(i18n_.t("status.no_image"));
        return;
    }
    auto snap = image_.snapshot();
    int w = image_.width();
    int h = image_.currentLines();

    const char* home = std::getenv("HOME");
    if (!home) home = ".";

    char filename[256];
    std::time_t now = std::time(nullptr);
    std::tm* tm = std::localtime(&now);
    std::strftime(filename, sizeof(filename),
                  "/FAX_%Y%m%d_%H%M%S.png", tm);

    std::string path = std::string(home) + filename;
    std::string err = ImageIO::saveByExtension(path, snap.data(), w, h);
    if (err.empty()) {
        setStatus(std::string(i18n_.t("status.image_saved")) + ": " + path);
    } else {
        setStatus("Save failed: " + err);
    }
}

void App::_onExit() { wants_close_ = true; }

// ============================================================
// 录音
// ============================================================
void App::_onOpenRecord() { rec_win_.show(); }

void App::_onToggleRecord() {
    if (recording_) {
        recorder_.stop();
        recording_ = false;
        last_record_path_ = recorder_.tempPath();
        if (!last_record_path_.empty()) {
            // 直接保存到 HOME，避免额外对话框
            const char* home = std::getenv("HOME");
            if (!home) home = ".";
            char fn[256];
            std::time_t now = std::time(nullptr);
            std::tm* tm = std::localtime(&now);
            std::strftime(fn, sizeof(fn), "/FAX_record_%Y%m%d_%H%M%S.wav", tm);
            std::string path = std::string(home) + fn;
            if (recorder_.saveTo(path)) {
                setStatus(std::string(i18n_.t("status.recording_saved")) + ": " + path);
            } else {
                setStatus(i18n_.t("status.recording_done"));
            }
        } else {
            setStatus(i18n_.t("status.recording_done"));
        }
    } else {
        if (!recorder_.start(settings_.selected_audio_device, 48000, 1024)) {
            setStatus(i18n_.t("status.record_open_failed"));
            return;
        }
        recording_ = true;
        setStatus(i18n_.t("status.recording"));
    }
}

// ============================================================
// 关于 / 更新 / 设备
// ============================================================
void App::_onShowAbout() {
    _openModal(MODAL_ABOUT);
}

void App::_onCheckUpdate() {
    setStatus(i18n_.t("status.update_checking"));
    std::thread([this]() {
        UpdateInfo info = Updater::check("1.0.1");
        std::lock_guard<std::mutex> lk(update_result_mutex_);
        update_result_ = info;
        update_result_ready_ = true;
    }).detach();
}

void App::_onResetAudioDevice() {
    settings_.selected_audio_device = -1;
    setStatus(i18n_.t("status.audio_device_reset"));
}

void App::_onSelectAudioDevice() {
    modal_devices_ = PortAudioInput::listInputDevices();
    modal_selected_device_ = settings_.selected_audio_device;
    _openModal(MODAL_AUDIO_DEV);
}

// ============================================================
// 设置
// ============================================================
void App::_onSetSpectrumDecay() {
    modal_float_ = settings_.spectrum_decay_rate;
    _openModal(MODAL_SPECTRUM);
}

void App::_onSetImageWidth() {
    modal_int_ = settings_.image_width;
    _openModal(MODAL_WIDTH);
}

void App::_onShowContrast() {
    modal_float_ = settings_.contrast;
    modal_bool_ = settings_.is_binary_mode;
    _openModal(MODAL_CONTRAST);
}

void App::_onSetLanguage(const char* lang) {
    settings_.language = lang;
    i18n_.setLanguage(lang);
    main_win_.setLanguage(lang);
    SettingsIO::save(SettingsIO::defaultPath(), settings_);
    setStatus(i18n_.t("status.ready"));
}

void App::_onStartSkewCalib() {
    if (image_.currentLines() == 0) { setStatus(i18n_.t("status.no_image")); return; }
    is_manual_ = false;
    calib_win_.startSkewCalib();
}

void App::_onStartStartPointCalib() {
    if (image_.currentLines() == 0) { setStatus(i18n_.t("status.no_image")); return; }
    is_manual_ = false;
    calib_win_.startStartPointCalib();
}

void App::_onSetRpm(float sec) {
    settings_.seconds_per_line = sec;
    image_.clear();
    image_dirty_ = true;
    char buf[64];
    std::snprintf(buf, sizeof(buf), "%s: %.3f s",
                  i18n_.t("status.per_line"), sec);
    setStatus(buf);
}

void App::_onShowManual() {
    if (image_.currentLines() == 0) { setStatus(i18n_.t("status.no_image")); return; }
    if (state_ != State::IDLE) {
        setStatus(i18n_.t("status.manual_need_stop"));
        return;
    }
    is_manual_ = true;
    temp_skew_ = 0.0f;
    temp_offset_ = 0.0f;
    temp_split_line_ = 0;
    calib_win_.showManualMenu();
}

// ============================================================
// 模态框
// ============================================================
void App::_openModal(ModalId id) {
    modal_id_ = id;
    modal_need_open_ = true;
}

void App::_drawModals() {
    if (modal_need_open_) {
        switch (modal_id_) {
            case MODAL_SPECTRUM:   ImGui::OpenPopup("##spectrum_modal"); break;
            case MODAL_WIDTH:      ImGui::OpenPopup("##width_modal"); break;
            case MODAL_CONTRAST:   ImGui::OpenPopup("##contrast_modal"); break;
            case MODAL_ABOUT:      ImGui::OpenPopup("##about_modal"); break;
            case MODAL_UPDATE:     ImGui::OpenPopup("##update_modal"); break;
            case MODAL_AUDIO_DEV:  ImGui::OpenPopup("##audio_dev_modal"); break;
            case MODAL_SAVE_RECORD: ImGui::OpenPopup("##text_modal"); break;
            default: break;
        }
        modal_need_open_ = false;
    }

    if (modal_id_ == MODAL_SPECTRUM)   _drawModalSpectrum();
    if (modal_id_ == MODAL_WIDTH)      _drawModalWidth();
    if (modal_id_ == MODAL_CONTRAST)   _drawModalContrast();
    if (modal_id_ == MODAL_ABOUT)      _drawModalAbout();
    if (modal_id_ == MODAL_UPDATE)     _drawModalUpdate();
    if (modal_id_ == MODAL_AUDIO_DEV)  _drawModalAudioDev();
    if (modal_id_ == MODAL_SAVE_RECORD) _drawModalSaveRecord();
}

void App::_drawModalSpectrum() {
    if (!ImGui::BeginPopupModal("##spectrum_modal", nullptr,
                                ImGuiWindowFlags_AlwaysAutoResize)) return;
    ImGui::TextUnformatted("Spectrum decay (0.10 ~ 0.99):");
    ImGui::SetNextItemWidth(200);
    ImGui::InputFloat("##v", &modal_float_, 0.0f, 0.0f, "%.3f");
    if (ImGui::Button("OK", ImVec2(110, 0))) {
        if (modal_float_ >= 0.10f && modal_float_ <= 0.99f) {
            settings_.spectrum_decay_rate = modal_float_;
            char buf[64];
            std::snprintf(buf, sizeof(buf), "%s: %.3f",
                          i18n_.t("status.spectrum_decay"), modal_float_);
            setStatus(buf);
        }
        modal_id_ = MODAL_NONE;
        ImGui::CloseCurrentPopup();
    }
    ImGui::SameLine();
    if (ImGui::Button("Cancel", ImVec2(110, 0))) {
        modal_id_ = MODAL_NONE;
        ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
}

void App::_drawModalWidth() {
    if (!ImGui::BeginPopupModal("##width_modal", nullptr,
                                ImGuiWindowFlags_AlwaysAutoResize)) return;
    ImGui::TextUnformatted("Image width (200 ~ 8000):");
    ImGui::SetNextItemWidth(200);
    ImGui::InputInt("##v", &modal_int_);
    if (ImGui::Button("OK", ImVec2(110, 0))) {
        if (modal_int_ >= 200 && modal_int_ <= 8000) {
            settings_.image_width = modal_int_;
            image_.resizeWidth(modal_int_);
            image_.clear();
            image_tex_ = nullptr;
            main_win_.clearImageTexture();
            image_dirty_ = true;
            char buf[64];
            std::snprintf(buf, sizeof(buf), "Image width: %d", modal_int_);
            setStatus(buf);
        }
        modal_id_ = MODAL_NONE;
        ImGui::CloseCurrentPopup();
    }
    ImGui::SameLine();
    if (ImGui::Button("Cancel", ImVec2(110, 0))) {
        modal_id_ = MODAL_NONE;
        ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
}

void App::_drawModalContrast() {
    if (!ImGui::BeginPopupModal("##contrast_modal", nullptr,
                                ImGuiWindowFlags_AlwaysAutoResize)) return;
    ImGui::TextUnformatted("Contrast (0.10 ~ 5.00):");
    ImGui::SetNextItemWidth(200);
    ImGui::InputFloat("##v", &modal_float_, 0.0f, 0.0f, "%.2f");
    ImGui::Checkbox("Binarize (black/white only)", &modal_bool_);
    if (ImGui::Button("OK", ImVec2(110, 0))) {
        settings_.contrast = std::clamp(modal_float_, 0.10f, 5.00f);
        settings_.is_binary_mode = modal_bool_;
        demod_.setConfig({1500.0f, 2300.0f, 128, settings_.contrast});
        image_.reprocessAll(settings_.is_binary_mode);
        image_dirty_ = true;
        char buf[80];
        std::snprintf(buf, sizeof(buf), "Contrast: %.2f%s",
                      settings_.contrast,
                      settings_.is_binary_mode ? " [binary]" : "");
        setStatus(buf);
        modal_id_ = MODAL_NONE;
        ImGui::CloseCurrentPopup();
    }
    ImGui::SameLine();
    if (ImGui::Button("Cancel", ImVec2(110, 0))) {
        modal_id_ = MODAL_NONE;
        ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
}

void App::_drawModalAbout() {
    if (!ImGui::BeginPopupModal("##about_modal", nullptr,
                                ImGuiWindowFlags_AlwaysAutoResize)) return;
    ImGui::TextColored(ImVec4(1.0f, 0.92f, 0.23f, 1.0f),
                       "FAX Decoder for PC (C++ port)");
    ImGui::Text("Version  : 1.0.1");
    ImGui::Text("Author   : MC_blone");
    ImGui::Text("Original : 78-HAM/FAX-Decoder-for-PC");
    ImGui::TextDisabled("https://github.com/78-HAM/FAX-Decoder-for-PC");
    ImGui::Spacing();
    if (ImGui::Button("OK", ImVec2(110, 0))) {
        modal_id_ = MODAL_NONE;
        ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
}

void App::_drawModalUpdate() {
    if (!ImGui::BeginPopupModal("##update_modal", nullptr,
                                ImGuiWindowFlags_AlwaysAutoResize)) return;
    ImGui::TextUnformatted(modal_text_.c_str());
    if (ImGui::Button("Open browser", ImVec2(140, 0))) {
        // 用系统默认浏览器打开（Termux 下会失败，但 CI 平台可用）
        std::string cmd;
#ifdef _WIN32
        cmd = "start \"\" " + update_result_.latest_url;
#elif defined(__APPLE__)
        cmd = "open \"" + update_result_.latest_url + "\"";
#else
        cmd = "xdg-open \"" + update_result_.latest_url + "\" 2>/dev/null &";
#endif
        (void)std::system(cmd.c_str());
        modal_id_ = MODAL_NONE;
        ImGui::CloseCurrentPopup();
    }
    ImGui::SameLine();
    if (ImGui::Button("Close", ImVec2(110, 0))) {
        modal_id_ = MODAL_NONE;
        ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
}

void App::_drawModalAudioDev() {
    if (!ImGui::BeginPopupModal("##audio_dev_modal", nullptr,
                                ImGuiWindowFlags_AlwaysAutoResize)) return;
    ImGui::TextUnformatted("Select Audio Input Device:");
    if (modal_devices_.empty()) {
        ImGui::TextColored(ImVec4(1, 0.6f, 0.3f, 1),
                           "No input devices found.");
    } else {
        ImGui::BeginChild("##devlist", ImVec2(480, 200), true);
        for (auto& p : modal_devices_) {
            char label[256];
            std::snprintf(label, sizeof(label), "[%d] %s", p.first, p.second.c_str());
            bool sel = (modal_selected_device_ == p.first);
            if (ImGui::Selectable(label, sel)) {
                modal_selected_device_ = p.first;
            }
        }
        ImGui::EndChild();
    }
    if (ImGui::Button("OK", ImVec2(110, 0))) {
        if (modal_selected_device_ >= 0) {
            settings_.selected_audio_device = modal_selected_device_;
            for (auto& p : modal_devices_) {
                if (p.first == modal_selected_device_) {
                    setStatus(std::string(i18n_.t("status.device_selected")) + p.second);
                    break;
                }
            }
        }
        modal_id_ = MODAL_NONE;
        ImGui::CloseCurrentPopup();
    }
    ImGui::SameLine();
    if (ImGui::Button("Cancel", ImVec2(110, 0))) {
        modal_id_ = MODAL_NONE;
        ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
}

void App::_drawModalSaveRecord() {
    // 兜底文件路径输入（无 ImGuiFileDialog 时用）
    if (!ImGui::BeginPopupModal("##text_modal", nullptr,
                                ImGuiWindowFlags_AlwaysAutoResize)) return;
    ImGui::TextUnformatted("Enter audio file path:");
    ImGui::SetNextItemWidth(500);
    ImGui::InputText("##path", modal_buf_, sizeof(modal_buf_));
    if (ImGui::Button("OK", ImVec2(110, 0))) {
        if (modal_buf_[0]) {
            current_file_ = modal_buf_;
            has_file_ = true;
            main_win_.setHasFile(true);
            // 只取文件名
            std::string name = current_file_;
            auto slash = name.find_last_of("\\/");
            if (slash != std::string::npos) name = name.substr(slash + 1);
            main_win_.setFileName(name);
            setStatus(std::string(i18n_.t("status.loading_file")) + ": " + name);
        }
        modal_id_ = MODAL_NONE;
        ImGui::CloseCurrentPopup();
    }
    ImGui::SameLine();
    if (ImGui::Button("Cancel", ImVec2(110, 0))) {
        modal_id_ = MODAL_NONE;
        ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
}

// ============================================================
// 文件对话框（ImGuiFileDialog）
// ============================================================
void App::_drawFileDialog() {
#ifdef HAVE_IMGUI_FILE_DIALOG
    if (IGFD::FileDialog::Instance()->Display("ChooseAudio")) {
        if (IGFD::FileDialog::Instance()->IsOk()) {
            std::string filePathName = IGFD::FileDialog::Instance()->GetFilePathName();
            current_file_ = filePathName;
            has_file_ = true;
            main_win_.setHasFile(true);
            std::string name = current_file_;
            auto slash = name.find_last_of("\\/");
            if (slash != std::string::npos) name = name.substr(slash + 1);
            main_win_.setFileName(name);
            setStatus(std::string(i18n_.t("status.loading_file")) + ": " + name);
        }
        file_dialog_open_ = false;
        IGFD::FileDialog::Instance()->Close();
    }
#endif
}