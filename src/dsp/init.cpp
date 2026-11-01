#include "dsp/init.h"
#include <cmath>
#include <algorithm>
#include <cstring>

static constexpr float kPi = 3.14159265358979323846f;

FftKiss::FftKiss(int n) : n_(n) {
    if (n_ <= 0 || (n_ % 2) != 0) { n_ = 0; return; }
    cfg_ = kiss_fftr_alloc(n_, 0, nullptr, nullptr);
    if (!cfg_) { n_ = 0; return; }
    time_buf_.resize(n_);
    freq_buf_.resize(n_ / 2 + 1);
}
FftKiss::~FftKiss() { if (cfg_) { kiss_fftr_free(cfg_); cfg_ = nullptr; } }

void FftKiss::forwardReal(const float* input, float* out_re, float* out_im) {
    if (!cfg_ || !input) return;
    for (int i = 0; i < n_; ++i) time_buf_[i] = (kiss_fft_scalar)input[i];
    kiss_fftr(cfg_, time_buf_.data(), freq_buf_.data());
    int nb = n_ / 2 + 1;
    for (int i = 0; i < nb; ++i) {
        if (out_re) out_re[i] = (float)freq_buf_[i].r;
        if (out_im) out_im[i] = (float)freq_buf_[i].i;
    }
}
void FftKiss::forwardRealMagSq(const float* input, float* out_mag_sq) {
    if (!cfg_ || !input || !out_mag_sq) return;
    for (int i = 0; i < n_; ++i) time_buf_[i] = (kiss_fft_scalar)input[i];
    kiss_fftr(cfg_, time_buf_.data(), freq_buf_.data());
    int nb = n_ / 2 + 1;
    for (int i = 0; i < nb; ++i) {
        float r = (float)freq_buf_[i].r;
        float im = (float)freq_buf_[i].i;
        out_mag_sq[i] = r * r + im * im;
    }
}

Demodulator::Demodulator() { initWindow(); }
Demodulator::Demodulator(const Config& cfg) : cfg_(cfg) { initWindow(); }

void Demodulator::initWindow() {
    int N = cfg_.window_size;
    cfg_.window_size = N;
    window_.resize(N);
    // Python: 0.54 - 0.46 * np.cos(2.0 * np.pi * np.arange(N) / (N - 1))
    for (int i = 0; i < N; ++i) {
        window_[i] = 0.54f - 0.46f * std::cos(2.0f * kPi * (float)i / (float)(N - 1));
    }
}

void Demodulator::demodulateLine(const float* buf, int len, int sr,
                                 int out_width, float* out) const {
    if (!out || out_width <= 0) return;
    if (!buf || len < 8 || sr <= 0) {
        std::fill(out, out + out_width, 0.0f); // Python 默认返回 0
        return;
    }

    const int N = cfg_.window_size;
    if (!fft_ || fft_n_ != N) {
        fft_.reset(new FftKiss(N));
        fft_n_ = N;
        work_in_.resize(N);
        work_mag_.resize(fft_->nBins());
        window_.resize(N);
        for (int i = 0; i < N; ++i) {
            window_[i] = 0.54f - 0.46f * std::cos(2.0f * kPi * (float)i / (float)(N - 1));
        }
    }

    const int nb = fft_->nBins();
    const float samples_per_pixel = (float)len / (float)out_width;
    const float bin_hz = (float)sr / (float)N;

    int start_bin = std::max(1, (int)(cfg_.f_min / bin_hz) - 2);
    int end_bin   = std::min(nb - 2, (int)(cfg_.f_max / bin_hz) + 2);
    if (end_bin <= start_bin) end_bin = start_bin + 1;

    const float contrast = cfg_.contrast;

    for (int i = 0; i < out_width; ++i) {
        float center = (float)i * samples_per_pixel + samples_per_pixel * 0.5f;
        int   start  = (int)(center - (float)N * 0.5f);

        for (int j = 0; j < N; ++j) {
            int idx = start + j;
            float v = (idx >= 0 && idx < len) ? buf[idx] : 0.0f;
            work_in_[j] = v * window_[j];
        }

        fft_->forwardRealMagSq(work_in_.data(), work_mag_.data());

        int peak_bin = start_bin;
        float peak_val = work_mag_[start_bin];
        for (int b = start_bin + 1; b <= end_bin; ++b) {
            if (work_mag_[b] > peak_val) {
                peak_val = work_mag_[b];
                peak_bin = b;
            }
        }

        float freq = (float)peak_bin * bin_hz;
        if (peak_bin > 0 && peak_bin < nb - 1) {
            float alpha = work_mag_[peak_bin - 1];
            float beta  = work_mag_[peak_bin];
            float gamma = work_mag_[peak_bin + 1];
            float denom = alpha - 2.0f * beta + gamma + 1e-5f;
            float p = 0.5f * (alpha - gamma) / denom;
            freq = ((float)peak_bin + p) * bin_hz;
        }

        float v = (freq - cfg_.f_min) / (cfg_.f_max - cfg_.f_min);
        v = (v - (1.0f - contrast)) / std::max(contrast, 0.001f);
        v = std::clamp(v, 0.0f, 1.0f);
        out[i] = v;
    }
}

SpectrumDisplay::SpectrumDisplay(int fft_size) : fft_size_(fft_size), fft_(fft_size) {
    if (fft_size_ < 64) fft_size_ = 64;
    if ((fft_size_ % 2) != 0) fft_size_ += 1;
    ring_.assign(fft_size_, 0.0f);
    latest_.assign(fft_size_ / 2, 0.0f);
    display_.assign(fft_size_ / 2, 0.0f);
    window_.resize(fft_size_);
    tmp_work_.resize(fft_size_);
    tmp_mag_.resize(fft_size_ / 2 + 1);
    for (int i = 0; i < fft_size_; ++i) {
        window_[i] = 0.5f * (1.0f - std::cos(2.0f * kPi * (float)i / (float)(fft_size_ - 1)));
    }
}

void SpectrumDisplay::pushSamples(const float* samples, int n) {
    if (!samples || n <= 0) return;
    int pos = 0;
    while (pos < n) {
        int space = fft_size_ - ring_pos_;
        int take  = std::min(space, n - pos);
        std::memcpy(&ring_[ring_pos_], samples + pos, take * sizeof(float));
        ring_pos_ += take;
        pos += take;
        if (ring_pos_ >= fft_size_) {
            computeSpectrum();
            ring_pos_ = 0;
        }
    }
}

void SpectrumDisplay::computeSpectrum() {
    for (int i = 0; i < fft_size_; ++i) {
        tmp_work_[i] = (ring_[i] / 32768.0f) * window_[i];
    }
    fft_.forwardRealMagSq(tmp_work_.data(), tmp_mag_.data());

    int nb_half = fft_size_ / 2;
    float max_db = -1e9f;
    for (int i = 0; i < nb_half; ++i) {
        float abs_val = std::sqrt(tmp_mag_[i]);
        float d = 20.0f * std::log10(abs_val + 1e-4f);
        tmp_work_[i] = d;
        if (d > max_db) max_db = d;
    }
    for (int i = 0; i < nb_half; ++i) {
        float norm = (tmp_work_[i] - (max_db - 60.0f)) / 60.0f;
        norm = std::clamp(norm, 0.0f, 1.0f);
        if (norm > latest_[i]) latest_[i] = norm;
    }
}

void SpectrumDisplay::updateDecay(float decay_rate) {
    int nb = fft_size_ / 2;
    for (int i = 0; i < nb; ++i) {
        display_[i] *= decay_rate;
        if (display_[i] < 0.001f) display_[i] = 0.0f;
        if (latest_[i] > display_[i]) display_[i] = latest_[i];
        latest_[i] *= 0.99f;
    }
}

void SpectrumDisplay::reset() {
    std::fill(ring_.begin(), ring_.end(), 0.0f);
    std::fill(latest_.begin(), latest_.end(), 0.0f);
    std::fill(display_.begin(), display_.end(), 0.0f);
    ring_pos_ = 0;
}

namespace DspUtil {
void resampleSpectrumToBars(const float* spec, int n_bins, int sr, int fft_size, float max_hz, float* out, int n_out) {
    if (!spec || n_bins <= 0 || sr <= 0 || fft_size <= 0 || n_out <= 0 || !out) return;
    const float bin_hz = (float)sr / (float)fft_size;
    int active = std::min(n_bins - 1, (int)(max_hz / std::max(bin_hz, 1e-6f)));
    if (active <= 0) { std::fill(out, out + n_out, 0.0f); return; }
    for (int i = 0; i < n_out; ++i) {
        float t = ((float)i + 0.5f) / (float)n_out * (float)active;
        int j = (int)t;
        if (j < 0) j = 0;
        if (j >= active) j = active - 1;
        out[i] = spec[j];
    }
}
}
