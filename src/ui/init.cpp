#include "ui/init.h"
#include <unordered_map>
#include <algorithm>
#include <cmath>
#include <cstdio>

// ============================================================
// 翻译表
// ============================================================
static const std::pair<const char*, const char*> kLangList[] = {
    {"en",    "en"},
    {"zh-cn", "zh-cn"},
    {"zh-tw", "zh-tw"},
    {"jp",    "jp"},
    {"ru",    "ru"},
};
int langIndex(const std::string& code) {
    if (code == "en")    return 0;
    if (code == "zh-cn" || code == "zh_CN") return 1;
    if (code == "zh-tw" || code == "zh_TW") return 4;
    if (code == "jp" || code == "ja")    return 3;
    if (code == "ru")    return 2;
    return 0;
}

using TR5 = std::array<const char*, 5>;
const std::unordered_map<std::string, TR5>& trTable() {
    static const std::unordered_map<std::string, TR5> m = {
        {"app.title", {"FAX Decoder 2.0", "FAX 解码器 2.0", "FAX Декодер 2.0", "FAX デコーダー 2.0", "FAX 解碼器 2.0"}},
        {"menu.file", {"File", "文件", "Файл", "ファイル", "檔案"}},
        {"menu.file.pick", {"Select Audio File...", "选择音频文件…", "Выбрать аудиофайл…", "音声ファイルを選択…", "選擇音訊檔案…"}},
        {"menu.file.decode", {"Start File Decoding", "开始文件解码", "Начать декодирование", "ファイルデコード開始", "開始檔案解碼"}},
        {"menu.file.save", {"Save Image...", "保存图像…", "Сохранить изображение…", "画像を保存…", "儲存圖片…"}},
        {"menu.file.exit", {"Exit", "退出", "Выход", "終了", "結束"}},
        {"menu.settings", {"Settings", "设置", "Настройки", "設定", "設定"}},
        {"menu.settings.font", {"Load Custom Font...", "加载自定义字体…", "Загрузить шрифт…", "カスタムフォントを読込…", "載入自訂字體…"}},
        {"menu.settings.rpm", {"RPM", "转速", "Обороты", "回転数", "轉速"}},
        {"menu.settings.lang", {"Language", "语言", "Язык", "言語", "語言"}},
        {"menu.settings.drivers", {"Audio Drivers", "音频驱动", "Аудио драйверы", "オーディオドライバー", "音訊驅動"}},
        {"menu.calib", {"Calibration", "校准", "Калибровка", "キャリブレーション", "校正"}},
        {"menu.calib.global", {"Global Calibration", "全局校准", "Глобальная калибровка", "全体キャリブレーション", "全域校正"}},
        {"menu.calib.global.skew", {"Calibrate Skew", "校准歪斜", "Калибровка наклона", "傾き補正", "校正歪斜"}},
        {"menu.calib.global.start", {"Set Start Point", "设置起点", "Задать начало", "開始位置を設定", "設定起點"}},
        {"menu.calib.manual", {"Manual Correction", "手动纠错", "Ручная коррекция", "手動補正", "手動修正"}},
        {"menu.record", {"Recording", "录制", "Запись", "録音", "錄製"}},
        {"menu.about", {"About", "关于", "О программе", "このアプリについて", "關於"}},
        {"menu.about.check_update", {"Check for Updates", "检查更新", "Проверить обновления", "更新を確認", "檢查更新"}},
        {"btn.start", {"Start Decoding", "开始解码", "Старт", "デコード開始", "開始解碼"}},
        {"btn.stop", {"Stop Decoding", "停止解码", "Стоп", "デコード停止", "停止解碼"}},
        {"btn.clear", {"Clear Image", "清空图像", "Очистить", "クリア", "清空圖片"}},
        {"label.spectrum", {"Spectrum", "频谱", "Спектр", "スペクトル", "頻譜"}},
        {"label.status", {"Status:", "状态：", "Статус:", "状態：", "狀態："}},
        {"status.ready", {"Ready", "就绪", "Готов", "準備完了", "就緒"}},
        {"status.manual_saved", {"Manual RAW saved: ", "手动 RAW 已保存：", "Ручной RAW сохранён: ", "手動 RAW 保存済: ", "手動 RAW 已儲存："}},
        {"calib.btn.fine_tune", {"Fine-tune", "手动调整", "Точная настройка", "微調整", "手動調整"}},
        {"calib.btn.confirm_p1", {"Confirm Point 1", "确定第 1 点", "Подтвердить точку 1", "点1を確定", "確定第 1 點"}},
        {"calib.btn.apply", {"Apply", "应用", "Применить", "適用", "套用"}},
        {"calib.btn.back", {"Back", "返回", "Назад", "戻る", "返回"}},
        {"calib.btn.cancel", {"Cancel", "取消", "Отмена", "キャンセル", "取消"}},
        {"calib.btn.confirm_pos", {"Confirm Position", "确定位置", "Подтвердить позицию", "位置を確定", "確定位置"}},
    };
    return m;
}

const char* MainWindow::t(const char* key) const {
    const auto& m = trTable();
    auto it = m.find(key);
    if (it == m.end()) return key;
    return it->second[langIndex(language_)];
}

// ============================================================
// MainWindow
// ============================================================
void MainWindow::draw() {
    drawMenuBar();

    ImGuiViewport* vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(vp->WorkPos);
    ImGui::SetNextWindowSize(vp->WorkSize);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::Begin("##main", nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus |
        ImGuiWindowFlags_NoScrollbar);
    ImGui::PopStyleVar();

    ImVec2 avail = ImGui::GetContentRegionAvail();
    float right_w = 340.0f;
    float left_w  = std::max(100.0f, avail.x - right_w);

    ImGui::BeginChild("##canvas", ImVec2(left_w, 0), false,
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    drawCanvas();
    ImGui::EndChild();

    ImGui::SameLine();

    ImGui::BeginChild("##right", ImVec2(0, 0), false);
    drawRightPanel();
    ImGui::EndChild();

    ImGui::End();

    drawCalibBar();
}

void MainWindow::drawMenuBar() {
    if (!ImGui::BeginMainMenuBar()) return;

    if (ImGui::BeginMenu(t("menu.file"))) {
        if (ImGui::MenuItem(t("menu.file.pick"))) { if (cb_.onPickFile) cb_.onPickFile(); }
        if (ImGui::MenuItem(t("menu.file.decode"), nullptr, false, has_file_)) { if (cb_.onStartFileDecode) cb_.onStartFileDecode(); }
        ImGui::Separator();
        if (ImGui::MenuItem(t("menu.file.save"), nullptr, false, has_image_)) { if (cb_.onSaveImage) cb_.onSaveImage(); }
        ImGui::Separator();
        if (ImGui::MenuItem(t("menu.file.exit"))) { if (cb_.onExit) cb_.onExit(); }
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu(t("menu.settings"))) {
        if (ImGui::MenuItem(t("menu.settings.font"))) { if (cb_.onLoadFont) cb_.onLoadFont(); }
        
        if (ImGui::BeginMenu(t("menu.settings.rpm"))) {
            struct R { const char* label; float sec; } opts[] = { {"30 RPM", 2.0f}, {"60 RPM", 1.0f}, {"120 RPM", 0.5f}, {"240 RPM", 0.25f}, {"480 RPM", 0.125f} };
            for (auto& o : opts) { if (ImGui::MenuItem(o.label)) { if (cb_.onSetRpm) cb_.onSetRpm(o.sec); } }
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu(t("menu.settings.lang"))) {
            for (auto& L : kLangList) {
                bool sel = (language_ == L.first);
                if (ImGui::MenuItem(L.second, nullptr, sel)) { if (cb_.onSetLanguage) cb_.onSetLanguage(L.first); }
            }
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu(t("menu.settings.drivers"))) {
            if (ImGui::MenuItem("Select Device...")) { if (cb_.onSelectAudioDevice) cb_.onSelectAudioDevice(); }
            ImGui::EndMenu();
        }
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu(t("menu.calib"))) {
        if (ImGui::BeginMenu(t("menu.calib.global"))) {
            if (ImGui::MenuItem(t("menu.calib.global.skew"))) { if (cb_.onStartSkewCalib) cb_.onStartSkewCalib(); }
            if (ImGui::MenuItem(t("menu.calib.global.start"))) { if (cb_.onStartStartPointCalib) cb_.onStartStartPointCalib(); }
            ImGui::EndMenu();
        }
        if (ImGui::MenuItem(t("menu.calib.manual"))) { if (cb_.onShowManual) cb_.onShowManual(); }
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu(t("menu.record"))) {
        if (ImGui::MenuItem(t("menu.record"))) { if (cb_.onOpenRecord) cb_.onOpenRecord(); }
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu(t("menu.about"))) {
        if (ImGui::MenuItem(t("menu.about.check_update"))) { if (cb_.onCheckUpdate) cb_.onCheckUpdate(); }
        ImGui::EndMenu();
    }

    ImGui::EndMainMenuBar();
}
void MainWindow::drawCanvas() {
    ImVec2 avail = ImGui::GetContentRegionAvail();
    ImVec2 p0 = ImGui::GetCursorScreenPos();

    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddRectFilled(p0, ImVec2(p0.x + avail.x, p0.y + avail.y), IM_COL32(0, 0, 0, 255));

    if (image_tex_ && image_w_ > 0 && image_h_ > 0) {
        float base = std::min(avail.x / image_w_, avail.y / image_h_);
        float s = base * view_scale_;
        float w = image_w_ * s;
        float h = image_h_ * s;
        float cx = p0.x + avail.x * 0.5f + view_offset_x_;
        float cy = p0.y + avail.y * 0.5f + view_offset_y_;
        ImVec2 i0(cx - w * 0.5f, cy - h * 0.5f);
        ImVec2 i1(i0.x + w, i0.y + h);

        image_screen_pos_  = i0;
        image_screen_size_ = ImVec2(w, h);

        dl->AddImage(image_tex_, i0, i1);
    } else {
        image_screen_pos_  = ImVec2(0, 0);
        image_screen_size_ = ImVec2(0, 0);

        const char* hint = "No image yet  /  等待图像数据";
        ImVec2 sz = ImGui::CalcTextSize(hint);
        dl->AddText(ImVec2(p0.x + (avail.x - sz.x) * 0.5f,
                           p0.y + (avail.y - sz.y) * 0.5f),
                    IM_COL32(120, 120, 120, 255), hint);
    }

    ImGui::InvisibleButton("##canvas_input", avail,
        ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight);
    bool hovered = ImGui::IsItemHovered();

    if (calib_mode_ == "NONE") {
        if (ImGui::IsItemActivated()) {
            dragging_   = true;
            drag_start_ = ImGui::GetIO().MousePos;
            drag_off_x_ = view_offset_x_;
            drag_off_y_ = view_offset_y_;
        }
        if (dragging_ && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            ImVec2 d = ImGui::GetIO().MousePos;
            view_offset_x_ = drag_off_x_ + (d.x - drag_start_.x);
            view_offset_y_ = drag_off_y_ + (d.y - drag_start_.y);
        }
        if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) dragging_ = false;

        if (hovered) {
            float wheel = ImGui::GetIO().MouseWheel;
            if (wheel != 0.0f) {
                float factor = std::pow(1.1f, wheel);
                view_scale_ = std::clamp(view_scale_ * factor, 0.05f, 20.0f);
            }
        }
    } else {
        if (hovered && image_screen_size_.x > 0) {
            ImVec2 m = ImGui::GetIO().MousePos;
            if (calib_mode_ == "SKEW_P1")          calib_p1 = m;
            else if (calib_mode_ == "SKEW_P2")     calib_p2 = m;
            else if (calib_mode_ == "START_POINT") calib_start_x = m.x;
            else if (calib_mode_ == "SPLIT_SELECT") calib_split_y = m.y;
        }
        if (calib_mode_ == "SPLIT_SELECT" &&
            ImGui::IsItemHovered() &&
            ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
            if (!calib_btns_.empty() && calib_btns_.back().cb) calib_btns_.back().cb();
        }
    }

    if (calib_mode_ != "NONE") {
        ImDrawList* fg = ImGui::GetForegroundDrawList();
        if (calib_mode_ == "SKEW_P1" || calib_mode_ == "SKEW_P2") {
            fg->AddLine(calib_p1, calib_p2, IM_COL32(0, 220, 0, 255), 2.0f);
            const float S = 25.0f;
            fg->AddLine(ImVec2(calib_p1.x - S, calib_p1.y), ImVec2(calib_p1.x + S, calib_p1.y), IM_COL32(0, 220, 0, 255), 3.0f);
            fg->AddLine(ImVec2(calib_p1.x, calib_p1.y - S), ImVec2(calib_p1.x, calib_p1.y + S), IM_COL32(0, 220, 0, 255), 3.0f);
            if (calib_mode_ == "SKEW_P2") {
                fg->AddLine(ImVec2(calib_p2.x - S, calib_p2.y), ImVec2(calib_p2.x + S, calib_p2.y), IM_COL32(255, 60, 60, 255), 3.0f);
                fg->AddLine(ImVec2(calib_p2.x, calib_p2.y - S), ImVec2(calib_p2.x, calib_p2.y + S), IM_COL32(255, 60, 60, 255), 3.0f);
            }
        } else if (calib_mode_ == "START_POINT") {
            fg->AddLine(ImVec2(calib_start_x, p0.y), ImVec2(calib_start_x, p0.y + avail.y),
                        IM_COL32(255, 235, 59, 255), 2.0f);
        } else if (calib_mode_ == "SPLIT_SELECT") {
            fg->AddLine(ImVec2(p0.x, calib_split_y), ImVec2(p0.x + avail.x, calib_split_y),
                        IM_COL32(255, 60, 60, 255), 3.0f);
        }
    }
}

void MainWindow::drawRightPanel() {
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
    float btn_w = (ImGui::GetContentRegionAvail().x - 8.0f) / 2.0f;
    ImVec2 btn_sz(btn_w, 60.0f);

    // 根据状态合并 Start / Stop 按钮
    if (decoding_) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.78f, 0.16f, 0.16f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.56f, 0.00f, 0.00f, 1.0f));
        if (ImGui::Button(t("btn.stop"), btn_sz)) { if (cb_.onToggleDecode) cb_.onToggleDecode(); }
        ImGui::PopStyleColor(2);
    } else {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.18f, 0.49f, 0.20f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.13f, 0.37f, 0.16f, 1.0f));
        if (ImGui::Button(t("btn.start"), btn_sz)) { if (cb_.onToggleDecode) cb_.onToggleDecode(); }
        ImGui::PopStyleColor(2);
    }
    
    ImGui::SameLine();

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.94f, 0.42f, 0.00f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.75f, 0.21f, 0.05f, 1.0f));
    if (ImGui::Button(t("btn.clear"), btn_sz)) { if (cb_.onClear) cb_.onClear(); }
    ImGui::PopStyleColor(2);

    ImGui::PopStyleVar();

    ImGui::Spacing();
    ImGui::TextUnformatted(t("label.spectrum"));
    drawSpectrum();

    ImGui::Separator();
    ImGui::TextUnformatted(t("label.status"));
    ImGui::TextWrapped("%s", status_.c_str());

    if (has_file_ && !file_name_.empty()) {
        ImGui::Separator();
        ImGui::TextDisabled("File: %s", file_name_.c_str());
    }
}

void MainWindow::drawSpectrum() {
    ImVec2 sz(ImGui::GetContentRegionAvail().x, 200.0f);
    ImVec2 p0 = ImGui::GetCursorScreenPos();
    ImVec2 p1(p0.x + sz.x, p0.y + sz.y);
    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddRectFilled(p0, p1, IM_COL32(0, 0, 0, 255));

    const int kBins = 128;
    static float spec[kBins];
    if (cb_.getSpectrum) cb_.getSpectrum(spec, kBins);

    float W = sz.x, H = sz.y;
    for (int i = 0; i < kBins; ++i) {
        float h = std::clamp(spec[i], 0.0f, 1.0f) * H * 0.9f;
        if (h < 1.0f) continue;
        float x = p0.x + (float)i / kBins * W;
        float bw = W / kBins;
        dl->AddRectFilled(ImVec2(x, p1.y - h), ImVec2(x + bw, p1.y),
                          IM_COL32(0, 255, 255, 255));
    }

    auto hz2x = [&](float hz) {
        return p0.x + std::clamp(hz / 4000.0f, 0.0f, 1.0f) * W;
    };
    float x1 = hz2x(1500.0f);
    float x2 = hz2x(2300.0f);
    dl->AddLine(ImVec2(x1, p0.y), ImVec2(x1, p1.y), IM_COL32(255, 60, 60, 255), 2.0f);
    dl->AddLine(ImVec2(x2, p0.y), ImVec2(x2, p1.y), IM_COL32(255, 60, 60, 255), 2.0f);
    dl->AddText(ImVec2(x1 + 2, p0.y + 2), IM_COL32(255, 255, 255, 255), "1500Hz");
    dl->AddText(ImVec2(x2 + 2, p0.y + 2), IM_COL32(255, 255, 255, 255), "2300Hz");

    dl->AddRect(p0, p1, IM_COL32(80, 80, 80, 255));
    ImGui::Dummy(sz);
}

void MainWindow::drawCalibBar() {
    if (!calib_bar_visible_) return;
    ImGuiViewport* vp = ImGui::GetMainViewport();
    float bar_h = 44.0f;
    ImGui::SetNextWindowPos(ImVec2(vp->WorkPos.x, vp->WorkPos.y + vp->WorkSize.y - bar_h));
    ImGui::SetNextWindowSize(ImVec2(vp->WorkSize.x, bar_h));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10, 8));
    ImGui::Begin("##calibbar", nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoBringToFrontOnFocus);
    ImGui::PopStyleVar();

    ImGui::TextUnformatted(calib_hint_.c_str());
    ImGui::SameLine(ImGui::GetWindowWidth() - 100.0f * (float)calib_btns_.size() - 20.0f);

    for (auto& b : calib_btns_) {
        if (ImGui::Button(b.label.c_str(), ImVec2(0, 26))) {
            if (b.cb) b.cb();
        }
        ImGui::SameLine();
    }
    ImGui::NewLine();
    ImGui::End();
}

// ============================================================
// RecordWindow
// ============================================================
void RecordWindow::draw() {
    if (!visible_) return;

    ImGui::SetNextWindowSize(ImVec2(360, 220), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin(title_.c_str(), &visible_)) { ImGui::End(); return; }

    bool rec = cb_.isRecording ? cb_.isRecording() : false;

    if (rec) {
        ImGui::TextColored(ImVec4(1.0f, 0.32f, 0.32f, 1.0f), "%s", recording_.c_str());
    } else {
        ImGui::TextColored(ImVec4(1.0f, 0.92f, 0.23f, 1.0f), "%s", idle_.c_str());
    }

    ImGui::Spacing();
    ImGui::Spacing();

    if (rec) {
        ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.78f, 0.16f, 0.16f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.56f, 0.00f, 0.00f, 1.0f));
        if (ImGui::Button(stop_.c_str(), ImVec2(-1, 40))) {
            if (cb_.onToggleRecord) cb_.onToggleRecord();
        }
        ImGui::PopStyleColor(2);
    } else {
        ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.18f, 0.49f, 0.20f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.13f, 0.37f, 0.16f, 1.0f));
        if (ImGui::Button(start_.c_str(), ImVec2(-1, 40))) {
            if (cb_.onToggleRecord) cb_.onToggleRecord();
        }
        ImGui::PopStyleColor(2);
    }

    ImGui::Spacing();
    ImGui::TextDisabled("%s", hint_.c_str());

    ImGui::End();
}

// ============================================================
// CalibWindow
// ============================================================
void CalibWindow::startSkewCalib() {
    if (!main_) return;
    if (hooks_.currentLineIndex && hooks_.currentLineIndex() == 0) {
        if (hooks_.setStatus) hooks_.setStatus("No image data");
        return;
    }
    mode_ = Mode::SKEW_P1;
    main_->setCalibMode("SKEW_P1");
    ImVec2 vp = ImGui::GetMainViewport()->WorkSize;
    main_->calib_p1 = ImVec2(vp.x * 0.5f, vp.y * 0.33f);
    main_->calib_p2 = main_->calib_p1;
    _refreshBar();
}

void CalibWindow::startStartPointCalib() {
    if (!main_) return;
    if (hooks_.currentLineIndex && hooks_.currentLineIndex() == 0) {
        if (hooks_.setStatus) hooks_.setStatus("No image data");
        return;
    }
    mode_ = Mode::START_POINT;
    main_->setCalibMode("START_POINT");
    main_->calib_start_x = ImGui::GetMainViewport()->WorkSize.x * 0.5f;
    _refreshBar();
}

void CalibWindow::showManualMenu() {
    if (!main_) return;
    if (hooks_.currentLineIndex && hooks_.currentLineIndex() == 0) {
        if (hooks_.setStatus) hooks_.setStatus("No image data");
        return;
    }
    mode_ = Mode::MANUAL_MENU;
    main_->setCalibMode("MANUAL");
    _refreshBar();
}

void CalibWindow::exitCalib() {
    mode_ = Mode::NONE;
    if (main_) {
        main_->setCalibMode("NONE");
        main_->hideCalibBar();
    }
    if (hooks_.setStatus) hooks_.setStatus("Calibration exited");
}

void CalibWindow::_refreshBar() {
    if (!main_) return;
    auto mk = [](const char* label, std::function<void()> cb) {
        return CalibButton{label, std::move(cb)};
    };

    switch (mode_) {
    case Mode::SKEW_P1:
        main_->showCalibBar("Skew: drag green cross to point 1",
            { mk("Fine-tune",        [this]() { if (hooks_.openFineTuneSkew) hooks_.openFineTuneSkew(); }),
              mk("Confirm Point 1",  [this]() { _skewNext(); }),
              mk("Cancel",           [this]() { exitCalib(); }) });
        break;

    case Mode::SKEW_P2:
        main_->showCalibBar("Skew: drag red cross to point 2",
            { mk("Fine-tune", [this]() { if (hooks_.openFineTuneSkew) hooks_.openFineTuneSkew(); }),
              mk("Apply",     [this]() { _applySkew(); }),
              mk("Back",      [this]() { mode_ = Mode::SKEW_P1; _refreshBar(); }) });
        break;

    case Mode::START_POINT:
        main_->showCalibBar("Set Start Point: drag yellow line",
            { mk("Fine-tune",        [this]() { if (hooks_.openFineTuneStart) hooks_.openFineTuneStart(); }),
              mk("Confirm Position", [this]() { _applyStartPoint(); }),
              mk("Cancel",           [this]() { exitCalib(); }) });
        break;

    case Mode::SPLIT_SELECT:
        main_->showCalibBar("Drag red horizontal line (release to confirm)",
            { mk("Cancel",  [this]() { showManualMenu(); }),
              mk("Confirm", [this]() { _finishSplit(); }) });
        break;

    case Mode::MANUAL_MENU:
        main_->showCalibBar("Manual Correction: choose action",
            { mk("1. Set Split",  [this]() { _startManualSplit(); }),
              mk("2. Set Offset", [this]() { _startManualStart(); }),
              mk("3. Set Skew",   [this]() { _startManualSkew(); }),
              mk("4. Apply",      [this]() { _commitManual(); }),
              mk("Back",          [this]() { exitCalib(); }) });
        break;

    case Mode::MANUAL_START:
        main_->showCalibBar("Manual: drag yellow line for lower start",
            { mk("Fine-tune",        [this]() { if (hooks_.openFineTuneStart) hooks_.openFineTuneStart(); }),
              mk("Confirm Position", [this]() { _applyStartPoint(); }),
              mk("Cancel",           [this]() { showManualMenu(); }) });
        break;

    case Mode::MANUAL_SKEW_P1:
        main_->showCalibBar("Manual: select point 1",
            { mk("Fine-tune",       [this]() { if (hooks_.openFineTuneSkew) hooks_.openFineTuneSkew(); }),
              mk("Confirm Point 1", [this]() { _manualSkewNext(); }),
              mk("Cancel",          [this]() { showManualMenu(); }) });
        break;

    case Mode::MANUAL_SKEW_P2:
        main_->showCalibBar("Manual: select point 2",
            { mk("Fine-tune", [this]() { if (hooks_.openFineTuneSkew) hooks_.openFineTuneSkew(); }),
              mk("Apply",     [this]() { _applyManualSkew(); }),
              mk("Back",      [this]() { _startManualSkew(); }) });
        break;

    default: break;
    }
}

void CalibWindow::_skewNext() {
    mode_ = Mode::SKEW_P2;
    if (main_) {
        main_->setCalibMode("SKEW_P2");
        if (main_->calib_p2.x == main_->calib_p1.x &&
            main_->calib_p2.y == main_->calib_p1.y) {
            main_->calib_p2 = ImVec2(main_->calib_p1.x + 200, main_->calib_p1.y + 200);
        }
    }
    _refreshBar();
}

void CalibWindow::_applySkew() {
    if (!main_ || !hooks_.applySkewDelta) { exitCalib(); return; }
    ImVec2 p1i = main_->canvasToImage(main_->calib_p1);
    ImVec2 p2i = main_->canvasToImage(main_->calib_p2);
    float dy = p2i.y - p1i.y;
    if (std::fabs(dy) < 1.0f) dy = 1.0f;
    float delta = -(p2i.x - p1i.x) / dy;
    hooks_.applySkewDelta(delta, p1i.y);
    exitCalib();
}

void CalibWindow::_applyStartPoint() {
    if (!main_) { exitCalib(); return; }
    if (hooks_.applyStartPoint) {
        ImVec2 pi = main_->canvasToImage(ImVec2(main_->calib_start_x, 0));
        hooks_.applyStartPoint(pi.x);
    }
    if (mode_ == Mode::MANUAL_START) showManualMenu();
    else                             exitCalib();
}

void CalibWindow::_finishSplit() {
    if (!main_) return;
    ImVec2 pi = main_->canvasToImage(ImVec2(0, main_->calib_split_y));
    int line = (int)std::lround(pi.y);
    if (line < 0) line = 0;
    if (hooks_.setTempSplitLine) hooks_.setTempSplitLine(line);
    if (hooks_.setStatus) {
        char buf[64];
        std::snprintf(buf, sizeof(buf), "Split line: %d", line);
        hooks_.setStatus(buf);
    }
    showManualMenu();
}

void CalibWindow::_startManualSplit() {
    mode_ = Mode::SPLIT_SELECT;
    if (main_) {
        main_->setCalibMode("SPLIT_SELECT");
        main_->calib_split_y = ImGui::GetMainViewport()->WorkSize.y * 0.5f;
    }
    _refreshBar();
}

void CalibWindow::_startManualStart() {
    mode_ = Mode::MANUAL_START;
    if (main_) {
        main_->setCalibMode("START_POINT");
        main_->calib_start_x = ImGui::GetMainViewport()->WorkSize.x * 0.5f;
    }
    _refreshBar();
}

void CalibWindow::_startManualSkew() {
    mode_ = Mode::MANUAL_SKEW_P1;
    if (main_) {
        main_->setCalibMode("SKEW_P1");
        ImVec2 vp = ImGui::GetMainViewport()->WorkSize;
        main_->calib_p1 = ImVec2(vp.x * 0.5f, vp.y * 0.33f);
        main_->calib_p2 = main_->calib_p1;
    }
    _refreshBar();
}

void CalibWindow::_manualSkewNext() {
    mode_ = Mode::MANUAL_SKEW_P2;
    if (main_) {
        main_->setCalibMode("SKEW_P2");
        if (main_->calib_p2.x == main_->calib_p1.x &&
            main_->calib_p2.y == main_->calib_p1.y) {
            main_->calib_p2 = ImVec2(main_->calib_p1.x + 200, main_->calib_p1.y + 200);
        }
    }
    _refreshBar();
}

void CalibWindow::_applyManualSkew() {
    if (!main_ || !hooks_.applySkewDelta) { showManualMenu(); return; }
    ImVec2 p1i = main_->canvasToImage(main_->calib_p1);
    ImVec2 p2i = main_->canvasToImage(main_->calib_p2);
    float dy = p2i.y - p1i.y;
    if (std::fabs(dy) < 1.0f) dy = 1.0f;
    float delta = -(p2i.x - p1i.x) / dy;
    hooks_.applySkewDelta(delta, p1i.y);
    showManualMenu();
}

void CalibWindow::_commitManual() {
    if (hooks_.commitManual) hooks_.commitManual();
    exitCalib();
}

// ============================================================
// FineTuneWindow
// ============================================================
void FineTuneWindow::open(bool skew_mode, float base_skew, float base_offset, int image_width) {
    skew_mode_     = skew_mode;
    base_skew_     = state_skew_   = base_skew;
    base_offset_   = state_offset_ = base_offset;
    image_width_   = image_width > 0 ? image_width : 2000;
    step_          = skew_mode ? 0.01f : 10.0f;
    manual_input_  = 0.0f;
    dragging_      = false;
    visible_       = true;
}

void FineTuneWindow::close() {
    visible_ = false;
    dragging_ = false;
}

void FineTuneWindow::draw() {
    if (!visible_) return;

    const char* title = skew_mode_ ? "Fine-tune - Skew" : "Fine-tune - Start Point";
    ImGui::SetNextWindowSize(ImVec2(1100, 850), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin(title, &visible_)) { ImGui::End(); return; }

    ImVec2 avail = ImGui::GetContentRegionAvail();
    float bottom_h = 190.0f;
    float pw = avail.x - 4.0f;
    float ph = avail.y - bottom_h - 4.0f;
    if (ph < 100.0f) ph = 100.0f;

    ImVec2 p0 = ImGui::GetCursorScreenPos();
    ImVec2 p1(p0.x + pw, p0.y + ph);

    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddRectFilled(p0, p1, IM_COL32(0, 0, 0, 255));
    dl->AddRect(p0, p1, IM_COL32(80, 80, 80, 255));

    if (snap_tex_ && snap_w_ > 0 && snap_h_ > 0) {
        float scale = std::min(pw / (float)snap_w_, ph / (float)snap_h_) * 0.98f;
        if (scale <= 0.0f) scale = 1.0f;
        float disp_w = snap_w_ * scale;
        float disp_h = snap_h_ * scale;
        float cx = p0.x + pw * 0.5f;
        float cy = p0.y + ph * 0.5f;
        ImVec2 i0(cx - disp_w * 0.5f, cy - disp_h * 0.5f);

        preview_origin_ = i0;
        preview_size_   = ImVec2(disp_w, disp_h);
        preview_scale_  = scale;

        float dx_total     = (state_offset_ - base_offset_) * scale;
        float shear_bottom = disp_h * (state_skew_ - base_skew_);

        ImVec2 q1(i0.x + dx_total,                          i0.y);
        ImVec2 q2(i0.x + dx_total + disp_w,                 i0.y);
        ImVec2 q3(i0.x + dx_total + disp_w + shear_bottom,  i0.y + disp_h);
        ImVec2 q4(i0.x + dx_total + shear_bottom,           i0.y + disp_h);

        dl->AddImageQuad(snap_tex_, q1, q2, q3, q4);
    }

    ImGui::InvisibleButton("##preview", ImVec2(pw, ph));

    if (ImGui::IsItemActivated()) {
        dragging_    = true;
        drag_start_  = ImGui::GetIO().MousePos;
        drag_skew_   = state_skew_;
        drag_offset_ = state_offset_;
    }
    if (dragging_ && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
        ImVec2 m = ImGui::GetIO().MousePos;
        float dx_canvas = m.x - drag_start_.x;
        float dy_canvas = m.y - drag_start_.y;
        if (preview_size_.x > 0.0f) {
            float orig_per_canvas_x = (float)image_width_ / preview_size_.x;
            state_offset_ = drag_offset_ + dx_canvas * orig_per_canvas_x;
        }
        if (skew_mode_ && preview_size_.y > 0.0f) {
            state_skew_ = drag_skew_ + dy_canvas / preview_size_.y;
        }
    }
    if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) dragging_ = false;

    {
        char info[200];
        std::snprintf(info, sizeof(info),
            "skew = %.5f    offset = %.2f\n"
            "Drag: horizontal=offset (+right / -left), vertical=skew (+down / -up)",
            state_skew_, state_offset_);
        dl->AddText(ImVec2(p0.x + 8, p0.y + 8), IM_COL32(255, 235, 59, 255), info);
    }

    ImGui::SetCursorScreenPos(ImVec2(p0.x, p1.y + 8));

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.08f, 0.40f, 0.75f, 1.0f));
    if (ImGui::Button("<<", ImVec2(64, 38))) {
        if (skew_mode_) state_skew_   -= step_;
        else            state_offset_ -= step_;
    }
    ImGui::PopStyleColor();

    ImGui::SameLine();
    ImGui::TextUnformatted("Step:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(110);
    ImGui::InputFloat("##step", &step_, 0.0f, 0.0f, "%.5f");
    ImGui::SameLine();

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.08f, 0.40f, 0.75f, 1.0f));
    if (ImGui::Button(">>", ImVec2(64, 38))) {
        if (skew_mode_) state_skew_   += step_;
        else            state_offset_ += step_;
    }
    ImGui::PopStyleColor();

    ImGui::Spacing();

    ImGui::TextUnformatted("Manual:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(170);
    ImGui::InputFloat("##manual", &manual_input_, 0.0f, 0.0f, "%.5f");
    ImGui::SameLine();
    if (ImGui::Button("Apply##manual", ImVec2(100, 28))) {
        if (skew_mode_) state_skew_   += manual_input_;
        else            state_offset_ += manual_input_;
        manual_input_ = 0.0f;
    }

    ImGui::Spacing();
    ImGui::Spacing();

    if (ImGui::Button("Reset", ImVec2(130, 34))) {
        state_skew_   = base_skew_;
        state_offset_ = base_offset_;
    }

    float right_align = ImGui::GetWindowWidth() - 320.0f;
    if (right_align > ImGui::GetCursorPosX()) ImGui::SameLine(right_align);

    if (ImGui::Button("Cancel", ImVec2(130, 34))) {
        if (cb_.onCancel) cb_.onCancel();
        visible_ = false;
    }

    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.00f, 0.55f, 0.52f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.00f, 0.68f, 0.64f, 1.0f));
    if (ImGui::Button("Confirm", ImVec2(150, 34))) {
        if (cb_.onApply) cb_.onApply(state_skew_, state_offset_);
        visible_ = false;
    }
    ImGui::PopStyleColor(2);

    ImGui::End();
}