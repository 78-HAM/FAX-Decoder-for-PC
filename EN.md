# FAX Decoder

A Python-based real-time fax signal decoder that demodulates fax signals (1500Hz ~ 2300Hz frequency-shift keying) from audio into grayscale images. It supports real-time sound card input, audio file decoding, spectrum display, skew calibration, and manual correction.

中文文档请参见 [README.md](README.md).

---
BY MC_blone
---
## Features

- **Real-time decoding**: Capture and decode audio from a sound card (microphone / virtual loopback) in real time
- **File decoding**: Supports WAV / FLAC / OGG / AIFF / AU / MP3 / M4A and other audio formats
- **Spectrum display**: Real-time FFT spectrum visualization with markers at 1500Hz / 2300Hz
- **Image export**: PNG / JPEG / TIFF / 16-bit RAW grayscale
- **Audio recording**: Record raw audio for later analysis
- **Skew calibration**: Two-point manual correction of image skew
- **Start point calibration**: Drag a vertical line to set the scan start offset
- **Manual segmented correction**: Independently adjust offset and skew for different segments of a decoded image and commit them to history
- **Multi-speed support**: 30 / 60 / 120 / 240 / 480 RPM
- **Contrast / binarization**: Adjust contrast in real time and output pure black-and-white images
- **Zoom & pan**: Mouse wheel to zoom, drag to pan

---

## How It Works

Fax signals use **FSK (Frequency-Shift Keying)** modulation:

| Frequency | Meaning |
|-----------|---------|
| 1500 Hz | White (low brightness) |
| 2300 Hz | Black (high brightness) |

Decoding pipeline:

Audio input -> Resample (to 60kHz) -> Segment by line -> Per-pixel FFT peak detection
             -> Frequency-to-grayscale mapping -> Apply skew/offset -> Image bitmap

The program performs an FFT on each pixel window, finds the spectral peak within 1500–2300Hz, refines the frequency using parabolic interpolation, and linearly maps it to 0–255 grayscale.

---

## Installation

### Requirements

- Python 3.8+
- Windows / macOS / Linux

### Install dependencies

pip install numpy pyaudio soundfile pillow

> **PyAudio installation notes**
>
> - **Windows**: `pip install pyaudio` usually works directly
> - **Linux**: First install `sudo apt install portaudio19-dev python3-pyaudio`, then `pip install pyaudio`
> - **macOS**: `brew install portaudio && pip install pyaudio`

---

## Quick Start

python fax_decoder.py

---

## Usage Guide

### 1. Select Audio Input Device (Real-time Decoding)

Menu bar -> **Drivers** -> **Settings…**

Choose a suitable input device from the list:

- Decode audio picked up by a microphone: select the microphone
- Decode audio played by the system (e.g., fax audio from SDR or a media player):
  - Windows: **Stereo Mix** or **VB-Cable**
  - macOS: **BlackHole** / **Loopback**
  - Linux: **PulseAudio Monitor**

Click **OK**. If unsure, click **Reset** to use the system default device.

### 2. Select RPM

Menu bar -> **Settings** -> **Select RPM**

| RPM | Seconds per line | Typical use |
|-----|------------------|-------------|
| 60 RPM | 1.0 s | Standard weather fax |
| 120 RPM | 0.5 s | Most radio weather fax (default) |
| 240 RPM | 0.25 s | High-speed fax |

> An incorrect RPM will stretch or compress the image.

### 3. Adjust Image Width

Menu bar -> **Settings** -> **Set Image Width** (default 2000 pixels)

The width generally corresponds to the number of samples per line in the source. Adjust if the image is clearly distorted.

### 4. Start Decoding

#### Real-time decoding
1. Click the **Start Decoding** button on the right
2. Play the audio or transmit the fax signal into the microphone
3. Check whether there is energy between 1500–2300Hz in the spectrum
4. Click **Stop Decoding** when finished

#### File decoding
1. Menu bar -> **File** -> **Select Audio File…**
2. Menu bar -> **File** -> **Start File Decoding**
3. A notification appears when decoding completes

### 5. Image Calibration

#### Skew calibration (global)
Menu bar -> **Settings** -> **Calibrate Skew (Global)**

1. Find a **reference point that should be horizontally aligned** on the canvas (e.g., a feature on the image edge)
2. Drag the green crosshair to point 1 and click **Confirm Point 1**
3. Drag the red crosshair to the position where the same reference point appears lower in the image
4. Click **Apply Calibration**

#### Set start point (global)
Menu bar -> **Settings** -> **Set Start Point (Global)**

Drag the yellow vertical line to the actual starting position of the image content, then click **Confirm Position**.

#### Manual correction (segmented calibration)
Menu bar -> **Settings** -> **Manual Correction**

Useful when one segment of the image is skewed while others are fine:

1. **Set split line**: Drag the red horizontal line on the canvas to choose the split position
2. **Set offset**: Drag the yellow vertical line to adjust the start of the lower segment
3. **Set skew**: Use the two-point method to measure the skew of the lower segment
4. **Confirm apply**: Commit the adjustment to the history data

> At any step you can open the fine-tuning numeric window via **Manual Adjust** and use button stepping (left / right) or enter values manually.

### 6. Adjust Contrast

Menu bar -> **Settings** -> **Set Contrast**

- Drag the slider to adjust contrast (0–500%)
- Check **Binarize** to output a pure black-and-white image
- Click **OK** to apply to all historical images in real time

### 7. Save Image

Menu bar -> **File** -> **Save Image…**

Supported formats:

| Format | Notes |
|--------|-------|
| PNG | Lossless grayscale (recommended) |
| JPEG | Lossy compression |
| TIFF | Lossless |
| RAW | 16-bit grayscale, requires width/line count to open |

### 8. Record Audio

Menu bar -> **Record** -> **Open Recording Panel**

1. Click **Start Recording**; the panel hides automatically (recording)
2. Reopen the panel from the menu
3. Click **Stop Recording** to open the save dialog

---

## Shortcuts / Controls

| Action | Effect |
|--------|--------|
| Left mouse drag | Pan the image |
| Mouse wheel | Zoom the image |
| Drag while calibrating | Move the calibration reference point |
| Esc (fine-tune window) | Close the window |

---

## Parameter Reference (top of the source)

SAMPLE_RATE_LIVE  = 48000   # Sound card sample rate
FFT_SIZE          = 2048    # FFT length for spectrum display
F_MIN, F_MAX      = 1500, 2300  # FSK frequency range (Hz)
TARGET_SAMPLE_RATE = 60000  # Internal resampling target for decoding
CHUNK_SIZE        = 1024    # Audio chunk size
MAX_LINES         = 10000   # Maximum image lines

### Adjustable UI parameters

- `self.image_width`: pixels per line (default 2000)
- `self.seconds_per_line`: seconds per line (default 0.5s = 120 RPM)
- `self.contrast`: contrast multiplier (default 1.1)
- `self.is_binary_mode`: whether to output a binarized image
- `self.spectrum_decay_rate`: spectrum decay rate (0.1 ~ 0.99)
- ---
