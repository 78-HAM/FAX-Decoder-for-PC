#pragma once
#include <vector>
#include <memory>
#include "kiss_fft.h"
#include "kiss_fftr.h"

// ============================================================
// FftKiss —— kissfft real-FFT 轻量封装
// ============================================================
class FftKiss {
public:
    explicit FftKiss(int n);
    ~FftKiss();
    FftKiss(const FftKiss&) = delete;
    FftKiss& operator=(const FftKiss&) = delete;

    int size() const { return n_; }
    int nBins() const { return n_ > 0 ? n_ / 2 + 1 : 0; }
    bool ok() const { return cfg_ != nullptr; }

    // 实数输入 n 点 → 复数输出 n/2+1 点
    void forwardReal(const float* input, float* out_re, float* out_im);
    // 实数输入 → 每 bin 的平方幅值（r^2 + i^2）
    void forwardRealMagSq(const float* input, float* out_mag_sq);

private:
    int n_ = 0;
    kiss_fftr_cfg cfg_ = nullptr;
    std::vector<kiss_fft_scalar> time_buf_;
    std::vector<kiss_fft_cpx>    freq_buf_;
};

// ============================================================
// Demodulator —— FAX 解调核心（对应 Python demodulate_fax）
// ============================================================
class Demodulator {
public:
    struct Config {
        float f_min = 1500.0f;       // Hz
        float f_max = 2300.0f;       // Hz
        int   window_size = 128;     // 每像素 FFT 点数
        float contrast = 1.1f;       // 对比度
    };

    Demodulator();
    explicit Demodulator(const Config& cfg);

    void setConfig(const Config& cfg) { cfg_ = cfg; }
    const Config& config() const { return cfg_; }

    // 输入样本值域不敏感（只影响峰值位置，不影响归一化）
    // 输出长度 = out_width，值 0..1
    void demodulateLine(const float* buf, int len, int sr, int out_width,
                        float* out) const;

private:
    void initWindow();

    Config cfg_;
    std::vector<float> window_;

    mutable std::unique_ptr<FftKiss> fft_;
    mutable int fft_n_ = 0;
    mutable std::vector<float> work_in_;
    mutable std::vector<float> work_mag_;
};

// ============================================================
// SpectrumDisplay —— 频谱累积 + 衰减显示（对应 Python 显示逻辑）
// ============================================================
class SpectrumDisplay {
public:
    explicit SpectrumDisplay(int fft_size = 2048);

    // 累积 int16 范围内的 float 样本（与 Python 端 /32768 保持一致）
    void pushSamples(const float* samples, int n);
    // 每帧调用一次
    void updateDecay(float decay_rate);
    void reset();

    const float* displayData() const { return display_.data(); }
    const float* latestData()  const { return latest_.data(); }
    int nBins()   const { return fft_size_ / 2; }
    int fftSize() const { return fft_size_; }

private:
    void computeSpectrum();

    int fft_size_;
    FftKiss fft_;
    std::vector<float> ring_;
    int ring_pos_ = 0;
    std::vector<float> latest_;
    std::vector<float> display_;
    std::vector<float> window_;
    std::vector<float> tmp_work_;
    std::vector<float> tmp_mag_;
};

// ============================================================
// DspUtil —— 常用小工具
// ============================================================
namespace DspUtil {
    // 把 SpectrumDisplay 的 n_bins 数据重采样成 UI 条状图（n_out 条）
    // src: displayData()；n_bins: nBins()；sr: 采样率；fft_size: fftSize()
    void resampleSpectrumToBars(const float* spec, int n_bins,
                                int sr, int fft_size,
                                float max_hz,
                                float* out, int n_out);
}