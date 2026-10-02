import os
import shutil
import tempfile
import threading
import webbrowser
import configparser
import tkinter as tk
from tkinter import filedialog, messagebox, simpledialog, Menu
from datetime import datetime
import wave
import urllib.request
import json
import re

import numpy as np
import pyaudio
import soundfile as sf
from PIL import Image, ImageTk


SAMPLE_RATE_LIVE = 48000
MAX_LINES = 10000
FFT_SIZE = 2048
F_MIN = 1500.0
F_MAX = 2300.0
TARGET_SAMPLE_RATE = 60000
CHUNK_SIZE = 1024

APP_VERSION = "1.0.1"
APP_AUTHOR = "MC_blone"
GITHUB_RELEASES_API = "https://api.github.com/repos/78-HAM/FAX-Decoder-for-PC/releases"
GITHUB_TAGS_API = "https://api.github.com/repos/78-HAM/FAX-Decoder-for-PC/tags"
GITHUB_RELEASES_PAGE = "https://github.com/78-HAM/FAX-Decoder-for-PC/releases"

SETTINGS_DIR = os.path.join(os.path.expanduser("~"), ".fax_decoder")
SETTINGS_FILE = os.path.join(SETTINGS_DIR, "setting.ini")

LANGUAGES = [
    ("zh_CN", "简体中文"),
    ("en", "English"),
    ("ru", "Русский"),
    ("ja", "日本語"),
    ("zh_TW", "繁體中文"),
]

TRANSLATIONS = {
    "zh_CN": {
        "app.title": "FAX Decoder",
        "menu.file": "文件",
        "menu.file.pick": "选择音频文件…",
        "menu.file.decode": "开始文件解码",
        "menu.file.save": "保存图像…",
        "menu.file.exit": "退出",
        "menu.settings": "设置",
        "menu.settings.spectrum_decay": "设置频谱衰减速度",
        "menu.settings.width": "设置图像宽度",
        "menu.settings.contrast": "设置对比度",
        "menu.settings.language": "语言",
        "menu.settings.skew": "校准歪斜 (全局)",
        "menu.settings.start": "设置起点 (全局)",
        "menu.settings.rpm": "选择转速",
        "menu.settings.manual": "手动纠正",
        "menu.drivers": "驱动",
        "menu.drivers.reset": "重置",
        "menu.drivers.settings": "设置…",
        "menu.record": "录制",
        "menu.record.open": "打开录制面板",
        "menu.about": "关于",
        "menu.about.about": "关于",
        "menu.about.check_update": "检查更新",

        "btn.start": "开始解码",
        "btn.stop": "停止解码",
        "btn.clear": "清空图像",
        "label.spectrum": "频谱",
        "label.status": "状态：",

        "status.ready": "就绪",
        "status.decoding": "正在解码…",
        "status.decoding_already": "解码已在进行中",
        "status.stopped": "已停止",
        "status.file_decoding": "文件解码中…",
        "status.file_done": "文件解码完成",
        "status.file_failed": "解码失败",
        "status.cleared": "已清空图像",
        "status.exited_calib": "已退出校准",
        "status.recording": "正在录制音频…",
        "status.recording_done": "录音结束",
        "status.recording_saved": "录音已保存",
        "status.save_cancelled": "已取消保存",
        "status.image_saved": "图像已保存",
        "status.manual_applied": "手动调整已应用",
        "status.audio_device_reset": "音频输入设备已重置为系统默认",
        "status.audio_device_selected": "已选设备",
        "status.per_line": "每行时长",
        "status.split_line": "分割线",
        "status.seconds": "秒",
        "status.line": "行",
        "status.spectrum_decay": "频谱衰减速度",
        "status.loading_file": "已选文件",

        "msg.no_image": "没有图像可保存",
        "msg.no_image_data": "尚无图像数据",
        "msg.empty_image": "图像为空",
        "msg.stop_first": "请先停止正在进行的解码",
        "msg.please_select_file": "请先选择音频文件",
        "msg.exit_calib_first": "请先退出当前校准",
        "msg.manual_need_stop": "手动纠正必须先在停止解码后才能进行",
        "msg.file_decode_running": "文件解码正在进行中",
        "msg.audio_open_failed": "无法打开音频流",
        "msg.recording_open_failed": "无法打开录音设备",
        "msg.temp_file_failed": "无法创建临时文件",
        "msg.decode_failed": "解码失败",
        "msg.save_failed": "保存失败",
        "msg.save_ok_title": "成功",
        "msg.pyaudio_failed": "PyAudio 初始化失败",
        "msg.no_input_device": "未找到可用的输入设备",
        "msg.calib_applied": "手动校准已应用并固化",

        "dlg.about.title": "关于",
        "dlg.about.author": "作者",
        "dlg.about.version": "当前版本",
        "dlg.update.failed_title": "检查更新",
        "dlg.update.failed_body": "检查失败，请检查网络连接",
        "dlg.update.latest_title": "检查更新",
        "dlg.update.latest_body": "当前已是最新版本",
        "dlg.update.found_title": "发现新版本",
        "dlg.update.found_body": "发现新版本",
        "dlg.update.current_version": "当前版本",
        "dlg.update.go": "是否前往下载？",
        "dlg.experimental.title": "实验性功能",
        "dlg.experimental.body": "这是一个实验性功能，可能不稳定。\n是否继续？",
        "dlg.restart.title": "切换语言",
        "dlg.restart.body": "切换语言为「{lang}」需要重启软件才能生效。\n\n是否立即重启？",
        "dlg.btn.ok": "确定",
        "dlg.btn.cancel": "取消",
        "dlg.btn.back": "返回",

        "dlg.save_audio.title": "保存录音",
        "dlg.save_audio.type": "WAV 音频",
        "dlg.save_image.title": "保存图像",
        "dlg.save_image.png": "PNG 图像",
        "dlg.save_image.jpg": "JPEG 图像",
        "dlg.save_image.tiff": "TIFF 图像",
        "dlg.save_image.raw": "RAW 16位灰度",

        "dlg.pick_audio.title": "选择音频文件",
        "dlg.pick_audio.type": "音频文件",
        "dlg.pick_audio.all": "所有文件",
        "dlg.pick_device.title": "选择音频输入设备",
        "dlg.pick_device.hint": "选择输入设备（虚拟声卡请选择 loopback / Stereo Mix / VB-Cable 等）",

        "dlg.spectrum_decay.title": "频谱衰减速度",
        "dlg.spectrum_decay.hint": "输入 0.1 ~ 0.99 之间的数值：",
        "dlg.width.title": "设置图像宽度",
        "dlg.width.hint": "像素：",
        "dlg.contrast.title": "调整对比度",
        "dlg.contrast.hint": "对比度百分比 (0 ~ 500)：",
        "dlg.contrast.binary": "二值化（仅黑/白）",
        "dlg.rpm.title": "选择转速",
        "dlg.record.title": "录制",
        "dlg.record.header": "录音控制",
        "dlg.record.start": "开始录制",
        "dlg.record.stop": "结束录制",
        "dlg.record.not_recording": "未在录制",
        "dlg.record.recording": "● 正在录制中…",
        "dlg.record.hint": "临时文件保存于系统临时目录，结束时可选择另存位置",

        "calib.skew.p1": "校准歪斜：拖动绿色十字选择第 1 点",
        "calib.skew.p2": "校准歪斜：拖动红色十字选择第 2 点（跟踪同一参考点）",
        "calib.start.hint": "设置起点：拖动黄色竖线选择新起点位置",
        "calib.manual.hint": "手动纠正：请选择操作步骤",
        "calib.manual.split": "拖动红色水平线选择分割位置（松开鼠标完成）",
        "calib.manual.start": "手动：拖动黄色竖线设置下段起点",
        "calib.manual.skew_p1": "手动：请选择第 1 点",
        "calib.manual.skew_p2": "手动：请选择第 2 点",
        "calib.btn.fine_tune": "手动调整",
        "calib.btn.confirm_p1": "确定第 1 点",
        "calib.btn.apply": "应用校准",
        "calib.btn.back": "返回",
        "calib.btn.cancel": "取消",
        "calib.btn.confirm_pos": "确定位置",
        "calib.btn.step1": "1. 设置分割线",
        "calib.btn.step2": "2. 设置偏移",
        "calib.btn.step3": "3. 设置斜度",
        "calib.btn.step4": "4. 确认应用",

        "fine.skew.title": "手动调整 - 歪斜",
        "fine.start.title": "手动调整 - 起点",
        "fine.step": "步长：",
        "fine.manual_value": "手动数值：",
        "fine.apply": "应用",
        "fine.reset": "重置",
        "fine.confirm_apply": "确认应用",
        "fine.hint": "拖动预览：水平=偏移（+右 / -左），垂直=斜度（+下 / -上）",
    },

    "en": {
        "app.title": "FAX Decoder",
        "menu.file": "File",
        "menu.file.pick": "Select Audio File…",
        "menu.file.decode": "Start File Decoding",
        "menu.file.save": "Save Image…",
        "menu.file.exit": "Exit",
        "menu.settings": "Settings",
        "menu.settings.spectrum_decay": "Set Spectrum Decay",
        "menu.settings.width": "Set Image Width",
        "menu.settings.contrast": "Set Contrast",
        "menu.settings.language": "Language",
        "menu.settings.skew": "Calibrate Skew (Global)",
        "menu.settings.start": "Set Start Point (Global)",
        "menu.settings.rpm": "Select RPM",
        "menu.settings.manual": "Manual Correction",
        "menu.drivers": "Drivers",
        "menu.drivers.reset": "Reset",
        "menu.drivers.settings": "Settings…",
        "menu.record": "Recording",
        "menu.record.open": "Open Recording Panel",
        "menu.about": "About",
        "menu.about.about": "About",
        "menu.about.check_update": "Check for Updates",

        "btn.start": "Start",
        "btn.stop": "Stop",
        "btn.clear": "Clear",
        "label.spectrum": "Spectrum",
        "label.status": "Status:",

        "status.ready": "Ready",
        "status.decoding": "Decoding…",
        "status.decoding_already": "Decoding already in progress",
        "status.stopped": "Stopped",
        "status.file_decoding": "Decoding file…",
        "status.file_done": "File decoding done",
        "status.file_failed": "Decode failed",
        "status.cleared": "Image cleared",
        "status.exited_calib": "Calibration exited",
        "status.recording": "Recording audio…",
        "status.recording_done": "Recording ended",
        "status.recording_saved": "Recording saved",
        "status.save_cancelled": "Save cancelled",
        "status.image_saved": "Image saved",
        "status.manual_applied": "Manual adjustment applied",
        "status.audio_device_reset": "Audio input device reset to system default",
        "status.audio_device_selected": "Device selected",
        "status.per_line": "Per line",
        "status.split_line": "Split line",
        "status.seconds": "s",
        "status.line": "line",
        "status.spectrum_decay": "Spectrum decay",
        "status.loading_file": "File selected",

        "msg.no_image": "No image to save",
        "msg.no_image_data": "No image data yet",
        "msg.empty_image": "Image is empty",
        "msg.stop_first": "Please stop the current decoding first",
        "msg.please_select_file": "Please select an audio file first",
        "msg.exit_calib_first": "Please exit the current calibration first",
        "msg.manual_need_stop": "Manual correction requires decoding to be stopped",
        "msg.file_decode_running": "File decoding is already running",
        "msg.audio_open_failed": "Failed to open audio stream",
        "msg.recording_open_failed": "Failed to open recording device",
        "msg.temp_file_failed": "Failed to create temp file",
        "msg.decode_failed": "Decode failed",
        "msg.save_failed": "Save failed",
        "msg.save_ok_title": "Success",
        "msg.pyaudio_failed": "PyAudio init failed",
        "msg.no_input_device": "No input device available",
        "msg.calib_applied": "Manual calibration applied and flattened",

        "dlg.about.title": "About",
        "dlg.about.author": "Author",
        "dlg.about.version": "Version",
        "dlg.update.failed_title": "Check for Updates",
        "dlg.update.failed_body": "Check failed. Please verify your network connection.",
        "dlg.update.latest_title": "Check for Updates",
        "dlg.update.latest_body": "You are already using the latest version",
        "dlg.update.found_title": "New Version Found",
        "dlg.update.found_body": "New version found",
        "dlg.update.current_version": "Current version",
        "dlg.update.go": "Go to download?",
        "dlg.experimental.title": "Experimental Feature",
        "dlg.experimental.body": "This is an experimental feature and may be unstable.\nContinue?",
        "dlg.restart.title": "Change Language",
        "dlg.restart.body": "Changing the language to \"{lang}\" requires restarting the application.\n\nRestart now?",
        "dlg.btn.ok": "OK",
        "dlg.btn.cancel": "Cancel",
        "dlg.btn.back": "Back",

        "dlg.save_audio.title": "Save Recording",
        "dlg.save_audio.type": "WAV Audio",
        "dlg.save_image.title": "Save Image",
        "dlg.save_image.png": "PNG Image",
        "dlg.save_image.jpg": "JPEG Image",
        "dlg.save_image.tiff": "TIFF Image",
        "dlg.save_image.raw": "RAW 16-bit Grayscale",

        "dlg.pick_audio.title": "Select Audio File",
        "dlg.pick_audio.type": "Audio files",
        "dlg.pick_audio.all": "All files",
        "dlg.pick_device.title": "Select Audio Input Device",
        "dlg.pick_device.hint": "Select an input device (choose loopback / Stereo Mix / VB-Cable for virtual audio)",

        "dlg.spectrum_decay.title": "Spectrum Decay",
        "dlg.spectrum_decay.hint": "Enter a value between 0.1 and 0.99:",
        "dlg.width.title": "Set Image Width",
        "dlg.width.hint": "Pixels:",
        "dlg.contrast.title": "Adjust Contrast",
        "dlg.contrast.hint": "Contrast percentage (0 ~ 500):",
        "dlg.contrast.binary": "Binarize (black/white only)",
        "dlg.rpm.title": "Select RPM",
        "dlg.record.title": "Recording",
        "dlg.record.header": "Recording Controls",
        "dlg.record.start": "Start Recording",
        "dlg.record.stop": "Stop Recording",
        "dlg.record.not_recording": "Not recording",
        "dlg.record.recording": "● Recording…",
        "dlg.record.hint": "Temp file stored in the system temp directory; save on stop",

        "calib.skew.p1": "Skew Calibration: drag the green cross to point 1",
        "calib.skew.p2": "Skew Calibration: drag the red cross to point 2 (tracking the same reference)",
        "calib.start.hint": "Set Start Point: drag the yellow vertical line",
        "calib.manual.hint": "Manual Correction: choose an action",
        "calib.manual.split": "Drag the red horizontal line to set the split position (release to confirm)",
        "calib.manual.start": "Manual: drag the yellow vertical line for the lower segment start",
        "calib.manual.skew_p1": "Manual: select point 1",
        "calib.manual.skew_p2": "Manual: select point 2",
        "calib.btn.fine_tune": "Fine-tune",
        "calib.btn.confirm_p1": "Confirm Point 1",
        "calib.btn.apply": "Apply",
        "calib.btn.back": "Back",
        "calib.btn.cancel": "Cancel",
        "calib.btn.confirm_pos": "Confirm Position",
        "calib.btn.step1": "1. Set Split Line",
        "calib.btn.step2": "2. Set Offset",
        "calib.btn.step3": "3. Set Skew",
        "calib.btn.step4": "4. Apply",

        "fine.skew.title": "Fine-tune - Skew",
        "fine.start.title": "Fine-tune - Start Point",
        "fine.step": "Step:",
        "fine.manual_value": "Manual value:",
        "fine.apply": "Apply",
        "fine.reset": "Reset",
        "fine.confirm_apply": "Confirm",
        "fine.hint": "Drag preview: horizontal=offset (+right / -left), vertical=skew (+down / -up)",
    },

    "ru": {
        "app.title": "FAX Decoder",
        "menu.file": "Файл",
        "menu.file.pick": "Выбрать аудиофайл…",
        "menu.file.decode": "Начать декодирование файла",
        "menu.file.save": "Сохранить изображение…",
        "menu.file.exit": "Выход",
        "menu.settings": "Настройки",
        "menu.settings.spectrum_decay": "Скорость затухания спектра",
        "menu.settings.width": "Ширина изображения",
        "menu.settings.contrast": "Контраст",
        "menu.settings.language": "Язык",
        "menu.settings.skew": "Калибровка наклона (глобально)",
        "menu.settings.start": "Задать начало (глобально)",
        "menu.settings.rpm": "Выбрать обороты",
        "menu.settings.manual": "Ручная коррекция",
        "menu.drivers": "Драйверы",
        "menu.drivers.reset": "Сброс",
        "menu.drivers.settings": "Настройки…",
        "menu.record": "Запись",
        "menu.record.open": "Открыть панель записи",
        "menu.about": "О программе",
        "menu.about.about": "О программе",
        "menu.about.check_update": "Проверить обновления",

        "btn.start": "Старт",
        "btn.stop": "Стоп",
        "btn.clear": "Очистить",
        "label.spectrum": "Спектр",
        "label.status": "Статус:",

        "status.ready": "Готов",
        "status.decoding": "Декодирование…",
        "status.decoding_already": "Декодирование уже идёт",
        "status.stopped": "Остановлено",
        "status.file_decoding": "Декодирование файла…",
        "status.file_done": "Декодирование завершено",
        "status.file_failed": "Ошибка декодирования",
        "status.cleared": "Изображение очищено",
        "status.exited_calib": "Калибровка завершена",
        "status.recording": "Запись аудио…",
        "status.recording_done": "Запись завершена",
        "status.recording_saved": "Запись сохранена",
        "status.save_cancelled": "Сохранение отменено",
        "status.image_saved": "Изображение сохранено",
        "status.manual_applied": "Ручная настройка применена",
        "status.audio_device_reset": "Устройство сброшено на системное по умолчанию",
        "status.audio_device_selected": "Выбрано устройство",
        "status.per_line": "Строка",
        "status.split_line": "Линия раздела",
        "status.seconds": "с",
        "status.line": "строка",
        "status.spectrum_decay": "Затухание спектра",
        "status.loading_file": "Выбран файл",

        "msg.no_image": "Нет изображения для сохранения",
        "msg.no_image_data": "Данных изображения пока нет",
        "msg.empty_image": "Изображение пустое",
        "msg.stop_first": "Сначала остановите текущее декодирование",
        "msg.please_select_file": "Сначала выберите аудиофайл",
        "msg.exit_calib_first": "Сначала выйдите из калибровки",
        "msg.manual_need_stop": "Ручная коррекция требует остановки декодирования",
        "msg.file_decode_running": "Декодирование файла уже идёт",
        "msg.audio_open_failed": "Не удалось открыть аудиопоток",
        "msg.recording_open_failed": "Не удалось открыть устройство записи",
        "msg.temp_file_failed": "Не удалось создать временный файл",
        "msg.decode_failed": "Ошибка декодирования",
        "msg.save_failed": "Ошибка сохранения",
        "msg.save_ok_title": "Успех",
        "msg.pyaudio_failed": "Ошибка инициализации PyAudio",
        "msg.no_input_device": "Нет доступных устройств ввода",
        "msg.calib_applied": "Ручная калибровка применена",

        "dlg.about.title": "О программе",
        "dlg.about.author": "Автор",
        "dlg.about.version": "Версия",
        "dlg.update.failed_title": "Проверка обновлений",
        "dlg.update.failed_body": "Проверка не удалась. Проверьте соединение.",
        "dlg.update.latest_title": "Проверка обновлений",
        "dlg.update.latest_body": "У вас уже последняя версия",
        "dlg.update.found_title": "Доступна новая версия",
        "dlg.update.found_body": "Найдена новая версия",
        "dlg.update.current_version": "Текущая версия",
        "dlg.update.go": "Перейти к загрузке?",
        "dlg.experimental.title": "Экспериментальная функция",
        "dlg.experimental.body": "Это экспериментальная функция, может работать нестабильно.\nПродолжить?",
        "dlg.restart.title": "Смена языка",
        "dlg.restart.body": "Для смены языка на «{lang}» требуется перезапуск приложения.\n\nПерезапустить сейчас?",
        "dlg.btn.ok": "OK",
        "dlg.btn.cancel": "Отмена",
        "dlg.btn.back": "Назад",

        "dlg.save_audio.title": "Сохранить запись",
        "dlg.save_audio.type": "Аудио WAV",
        "dlg.save_image.title": "Сохранить изображение",
        "dlg.save_image.png": "PNG",
        "dlg.save_image.jpg": "JPEG",
        "dlg.save_image.tiff": "TIFF",
        "dlg.save_image.raw": "RAW 16-bit градации серого",

        "dlg.pick_audio.title": "Выбрать аудиофайл",
        "dlg.pick_audio.type": "Аудиофайлы",
        "dlg.pick_audio.all": "Все файлы",
        "dlg.pick_device.title": "Выбор устройства ввода",
        "dlg.pick_device.hint": "Выберите устройство (для виртуального звука: loopback / Stereo Mix / VB-Cable)",

        "dlg.spectrum_decay.title": "Затухание спектра",
        "dlg.spectrum_decay.hint": "Введите значение от 0.1 до 0.99:",
        "dlg.width.title": "Ширина изображения",
        "dlg.width.hint": "Пиксели:",
        "dlg.contrast.title": "Настройка контраста",
        "dlg.contrast.hint": "Контраст в процентах (0 ~ 500):",
        "dlg.contrast.binary": "Бинаризация (только ч/б)",
        "dlg.rpm.title": "Выбор оборотов",
        "dlg.record.title": "Запись",
        "dlg.record.header": "Управление записью",
        "dlg.record.start": "Начать запись",
        "dlg.record.stop": "Остановить",
        "dlg.record.not_recording": "Не записывается",
        "dlg.record.recording": "● Запись…",
        "dlg.record.hint": "Временный файл в системном tmp; сохранение при остановке",

        "calib.skew.p1": "Калибровка наклона: перетащите зелёный крест к точке 1",
        "calib.skew.p2": "Калибровка наклона: перетащите красный крест к точке 2",
        "calib.start.hint": "Задать начало: перетащите жёлтую линию",
        "calib.manual.hint": "Ручная коррекция: выберите действие",
        "calib.manual.split": "Перетащите красную линию для позиции раздела (отпустите для подтверждения)",
        "calib.manual.start": "Вручную: перетащите жёлтую линию для начала нижнего сегмента",
        "calib.manual.skew_p1": "Вручную: выберите точку 1",
        "calib.manual.skew_p2": "Вручную: выберите точку 2",
        "calib.btn.fine_tune": "Точная настройка",
        "calib.btn.confirm_p1": "Подтвердить точку 1",
        "calib.btn.apply": "Применить",
        "calib.btn.back": "Назад",
        "calib.btn.cancel": "Отмена",
        "calib.btn.confirm_pos": "Подтвердить позицию",
        "calib.btn.step1": "1. Линия раздела",
        "calib.btn.step2": "2. Смещение",
        "calib.btn.step3": "3. Наклон",
        "calib.btn.step4": "4. Применить",

        "fine.skew.title": "Точная настройка - Наклон",
        "fine.start.title": "Точная настройка - Начало",
        "fine.step": "Шаг:",
        "fine.manual_value": "Значение:",
        "fine.apply": "Применить",
        "fine.reset": "Сброс",
        "fine.confirm_apply": "Подтвердить",
        "fine.hint": "Перетаскивание: горизонталь=смещение, вертикаль=наклон",
    },

    "ja": {
        "app.title": "FAX Decoder",
        "menu.file": "ファイル",
        "menu.file.pick": "音声ファイルを選択…",
        "menu.file.decode": "ファイルデコード開始",
        "menu.file.save": "画像を保存…",
        "menu.file.exit": "終了",
        "menu.settings": "設定",
        "menu.settings.spectrum_decay": "スペクトル減衰速度",
        "menu.settings.width": "画像幅を設定",
        "menu.settings.contrast": "コントラストを調整",
        "menu.settings.language": "言語",
        "menu.settings.skew": "傾き補正（全体）",
        "menu.settings.start": "開始位置を設定（全体）",
        "menu.settings.rpm": "回転数を選択",
        "menu.settings.manual": "手動補正",
        "menu.drivers": "ドライバー",
        "menu.drivers.reset": "リセット",
        "menu.drivers.settings": "設定…",
        "menu.record": "録音",
        "menu.record.open": "録音パネルを開く",
        "menu.about": "このアプリについて",
        "menu.about.about": "このアプリについて",
        "menu.about.check_update": "更新を確認",

        "btn.start": "開始",
        "btn.stop": "停止",
        "btn.clear": "クリア",
        "label.spectrum": "スペクトル",
        "label.status": "状態：",

        "status.ready": "準備完了",
        "status.decoding": "デコード中…",
        "status.decoding_already": "すでにデコード中です",
        "status.stopped": "停止しました",
        "status.file_decoding": "ファイルをデコード中…",
        "status.file_done": "ファイルデコード完了",
        "status.file_failed": "デコード失敗",
        "status.cleared": "画像をクリアしました",
        "status.exited_calib": "キャリブレーションを終了しました",
        "status.recording": "録音中…",
        "status.recording_done": "録音終了",
        "status.recording_saved": "録音を保存しました",
        "status.save_cancelled": "保存をキャンセルしました",
        "status.image_saved": "画像を保存しました",
        "status.manual_applied": "手動調整を適用しました",
        "status.audio_device_reset": "既定の入力デバイスにリセットしました",
        "status.audio_device_selected": "選択したデバイス",
        "status.per_line": "1行あたり",
        "status.split_line": "分割線",
        "status.seconds": "秒",
        "status.line": "行",
        "status.spectrum_decay": "スペクトル減衰",
        "status.loading_file": "選択ファイル",

        "msg.no_image": "保存する画像がありません",
        "msg.no_image_data": "画像データがまだありません",
        "msg.empty_image": "画像が空です",
        "msg.stop_first": "先にデコードを停止してください",
        "msg.please_select_file": "先に音声ファイルを選択してください",
        "msg.exit_calib_first": "先にキャリブレーションを終了してください",
        "msg.manual_need_stop": "手動補正はデコード停止後のみ可能です",
        "msg.file_decode_running": "ファイルデコードが進行中です",
        "msg.audio_open_failed": "オーディオストリームを開けません",
        "msg.recording_open_failed": "録音デバイスを開けません",
        "msg.temp_file_failed": "一時ファイルを作成できません",
        "msg.decode_failed": "デコード失敗",
        "msg.save_failed": "保存失敗",
        "msg.save_ok_title": "成功",
        "msg.pyaudio_failed": "PyAudio の初期化に失敗",
        "msg.no_input_device": "利用可能な入力デバイスがありません",
        "msg.calib_applied": "手動キャリブレーションを適用しました",

        "dlg.about.title": "このアプリについて",
        "dlg.about.author": "作者",
        "dlg.about.version": "バージョン",
        "dlg.update.failed_title": "更新を確認",
        "dlg.update.failed_body": "確認に失敗しました。ネットワーク接続を確認してください。",
        "dlg.update.latest_title": "更新を確認",
        "dlg.update.latest_body": "すでに最新バージョンです",
        "dlg.update.found_title": "新しいバージョンがあります",
        "dlg.update.found_body": "新しいバージョンを検出",
        "dlg.update.current_version": "現在のバージョン",
        "dlg.update.go": "ダウンロードしますか？",
        "dlg.experimental.title": "実験的機能",
        "dlg.experimental.body": "これは実験的機能で、不安定な場合があります。\n続行しますか？",
        "dlg.restart.title": "言語の切り替え",
        "dlg.restart.body": "言語を「{lang}」に変更するにはアプリの再起動が必要です。\n\n今すぐ再起動しますか？",
        "dlg.btn.ok": "OK",
        "dlg.btn.cancel": "キャンセル",
        "dlg.btn.back": "戻る",

        "dlg.save_audio.title": "録音を保存",
        "dlg.save_audio.type": "WAV オーディオ",
        "dlg.save_image.title": "画像を保存",
        "dlg.save_image.png": "PNG 画像",
        "dlg.save_image.jpg": "JPEG 画像",
        "dlg.save_image.tiff": "TIFF 画像",
        "dlg.save_image.raw": "RAW 16-bit グレースケール",

        "dlg.pick_audio.title": "音声ファイルを選択",
        "dlg.pick_audio.type": "音声ファイル",
        "dlg.pick_audio.all": "すべてのファイル",
        "dlg.pick_device.title": "音声入力デバイスの選択",
        "dlg.pick_device.hint": "入力デバイスを選択（仮想オーディオは loopback / Stereo Mix / VB-Cable など）",

        "dlg.spectrum_decay.title": "スペクトル減衰速度",
        "dlg.spectrum_decay.hint": "0.1 ～ 0.99 の範囲で入力：",
        "dlg.width.title": "画像幅の設定",
        "dlg.width.hint": "ピクセル：",
        "dlg.contrast.title": "コントラストの調整",
        "dlg.contrast.hint": "コントラスト % (0 ～ 500)：",
        "dlg.contrast.binary": "二値化（白黒のみ）",
        "dlg.rpm.title": "回転数の選択",
        "dlg.record.title": "録音",
        "dlg.record.header": "録音コントロール",
        "dlg.record.start": "録音開始",
        "dlg.record.stop": "録音停止",
        "dlg.record.not_recording": "録音していません",
        "dlg.record.recording": "● 録音中…",
        "dlg.record.hint": "一時ファイルはシステムの tmp に保存され、停止時に保存できます",

        "calib.skew.p1": "傾き補正：緑の十字を点1へドラッグ",
        "calib.skew.p2": "傾き補正：赤の十字を点2へドラッグ（同じ基準を追跡）",
        "calib.start.hint": "開始位置を設定：黄色の縦線をドラッグ",
        "calib.manual.hint": "手動補正：操作を選択",
        "calib.manual.split": "赤い水平線をドラッグして分割位置を決定（離して確定）",
        "calib.manual.start": "手動：黄色の縦線で下側セグメントの開始位置を指定",
        "calib.manual.skew_p1": "手動：点1を選択",
        "calib.manual.skew_p2": "手動：点2を選択",
        "calib.btn.fine_tune": "微調整",
        "calib.btn.confirm_p1": "点1を確定",
        "calib.btn.apply": "適用",
        "calib.btn.back": "戻る",
        "calib.btn.cancel": "キャンセル",
        "calib.btn.confirm_pos": "位置を確定",
        "calib.btn.step1": "1. 分割線",
        "calib.btn.step2": "2. オフセット",
        "calib.btn.step3": "3. 傾き",
        "calib.btn.step4": "4. 適用",

        "fine.skew.title": "微調整 - 傾き",
        "fine.start.title": "微調整 - 開始位置",
        "fine.step": "ステップ：",
        "fine.manual_value": "手動値：",
        "fine.apply": "適用",
        "fine.reset": "リセット",
        "fine.confirm_apply": "確定",
        "fine.hint": "ドラッグ：水平=オフセット、垂直=傾き",
    },

    "zh_TW": {
        "app.title": "FAX Decoder",
        "menu.file": "檔案",
        "menu.file.pick": "選擇音訊檔案…",
        "menu.file.decode": "開始檔案解碼",
        "menu.file.save": "儲存圖片…",
        "menu.file.exit": "結束",
        "menu.settings": "設定",
        "menu.settings.spectrum_decay": "設定頻譜衰減速度",
        "menu.settings.width": "設定圖片寬度",
        "menu.settings.contrast": "設定對比度",
        "menu.settings.language": "語言",
        "menu.settings.skew": "校正歪斜（全域）",
        "menu.settings.start": "設定起點（全域）",
        "menu.settings.rpm": "選擇轉速",
        "menu.settings.manual": "手動修正",
        "menu.drivers": "驅動程式",
        "menu.drivers.reset": "重設",
        "menu.drivers.settings": "設定…",
        "menu.record": "錄製",
        "menu.record.open": "開啟錄製面板",
        "menu.about": "關於",
        "menu.about.about": "關於",
        "menu.about.check_update": "檢查更新",

        "btn.start": "開始解碼",
        "btn.stop": "停止解碼",
        "btn.clear": "清空圖片",
        "label.spectrum": "頻譜",
        "label.status": "狀態：",

        "status.ready": "就緒",
        "status.decoding": "解碼中…",
        "status.decoding_already": "解碼已在進行中",
        "status.stopped": "已停止",
        "status.file_decoding": "檔案解碼中…",
        "status.file_done": "檔案解碼完成",
        "status.file_failed": "解碼失敗",
        "status.cleared": "已清空圖片",
        "status.exited_calib": "已離開校正",
        "status.recording": "正在錄製音訊…",
        "status.recording_done": "錄音結束",
        "status.recording_saved": "錄音已儲存",
        "status.save_cancelled": "已取消儲存",
        "status.image_saved": "圖片已儲存",
        "status.manual_applied": "手動調整已套用",
        "status.audio_device_reset": "音訊輸入裝置已重設為系統預設",
        "status.audio_device_selected": "已選裝置",
        "status.per_line": "每行時長",
        "status.split_line": "分割線",
        "status.seconds": "秒",
        "status.line": "行",
        "status.spectrum_decay": "頻譜衰減速度",
        "status.loading_file": "已選檔案",

        "msg.no_image": "沒有圖片可儲存",
        "msg.no_image_data": "尚無圖片資料",
        "msg.empty_image": "圖片為空",
        "msg.stop_first": "請先停止正在進行的解碼",
        "msg.please_select_file": "請先選擇音訊檔案",
        "msg.exit_calib_first": "請先離開目前的校正",
        "msg.manual_need_stop": "手動修正必須先停止解碼後才能進行",
        "msg.file_decode_running": "檔案解碼正在進行中",
        "msg.audio_open_failed": "無法開啟音訊串流",
        "msg.recording_open_failed": "無法開啟錄音裝置",
        "msg.temp_file_failed": "無法建立暫存檔",
        "msg.decode_failed": "解碼失敗",
        "msg.save_failed": "儲存失敗",
        "msg.save_ok_title": "成功",
        "msg.pyaudio_failed": "PyAudio 初始化失敗",
        "msg.no_input_device": "找不到可用的輸入裝置",
        "msg.calib_applied": "手動校正已套用並固化",

        "dlg.about.title": "關於",
        "dlg.about.author": "作者",
        "dlg.about.version": "目前版本",
        "dlg.update.failed_title": "檢查更新",
        "dlg.update.failed_body": "檢查失敗，請確認網路連線",
        "dlg.update.latest_title": "檢查更新",
        "dlg.update.latest_body": "目前已是最新版本",
        "dlg.update.found_title": "發現新版本",
        "dlg.update.found_body": "發現新版本",
        "dlg.update.current_version": "目前版本",
        "dlg.update.go": "是否前往下載？",
        "dlg.experimental.title": "實驗性功能",
        "dlg.experimental.body": "這是實驗性功能，可能不穩定。\n是否繼續？",
        "dlg.restart.title": "切換語言",
        "dlg.restart.body": "切換語言為「{lang}」需要重新啟動軟體才能生效。\n\n是否立即重新啟動？",
        "dlg.btn.ok": "確定",
        "dlg.btn.cancel": "取消",
        "dlg.btn.back": "返回",

        "dlg.save_audio.title": "儲存錄音",
        "dlg.save_audio.type": "WAV 音訊",
        "dlg.save_image.title": "儲存圖片",
        "dlg.save_image.png": "PNG 圖片",
        "dlg.save_image.jpg": "JPEG 圖片",
        "dlg.save_image.tiff": "TIFF 圖片",
        "dlg.save_image.raw": "RAW 16 位元灰階",

        "dlg.pick_audio.title": "選擇音訊檔案",
        "dlg.pick_audio.type": "音訊檔案",
        "dlg.pick_audio.all": "所有檔案",
        "dlg.pick_device.title": "選擇音訊輸入裝置",
        "dlg.pick_device.hint": "選擇輸入裝置（虛擬音效卡請選擇 loopback / Stereo Mix / VB-Cable 等）",

        "dlg.spectrum_decay.title": "頻譜衰減速度",
        "dlg.spectrum_decay.hint": "輸入 0.1 ~ 0.99 之間的數值：",
        "dlg.width.title": "設定圖片寬度",
        "dlg.width.hint": "像素：",
        "dlg.contrast.title": "調整對比度",
        "dlg.contrast.hint": "對比度百分比 (0 ~ 500)：",
        "dlg.contrast.binary": "二值化（僅黑/白）",
        "dlg.rpm.title": "選擇轉速",
        "dlg.record.title": "錄製",
        "dlg.record.header": "錄音控制",
        "dlg.record.start": "開始錄製",
        "dlg.record.stop": "結束錄製",
        "dlg.record.not_recording": "未在錄製",
        "dlg.record.recording": "● 正在錄製中…",
        "dlg.record.hint": "暫存檔存於系統暫存目錄，結束時可選擇另存位置",

        "calib.skew.p1": "校正歪斜：拖曳綠色十字選擇第 1 點",
        "calib.skew.p2": "校正歪斜：拖曳紅色十字選擇第 2 點（追蹤同一參考點）",
        "calib.start.hint": "設定起點：拖曳黃色垂直線選擇新起點位置",
        "calib.manual.hint": "手動修正：請選擇操作步驟",
        "calib.manual.split": "拖曳紅色水平線選擇分割位置（放開滑鼠完成）",
        "calib.manual.start": "手動：拖曳黃色垂直線設定下段起點",
        "calib.manual.skew_p1": "手動：請選擇第 1 點",
        "calib.manual.skew_p2": "手動：請選擇第 2 點",
        "calib.btn.fine_tune": "手動調整",
        "calib.btn.confirm_p1": "確定第 1 點",
        "calib.btn.apply": "套用校正",
        "calib.btn.back": "返回",
        "calib.btn.cancel": "取消",
        "calib.btn.confirm_pos": "確定位置",
        "calib.btn.step1": "1. 設定分割線",
        "calib.btn.step2": "2. 設定偏移",
        "calib.btn.step3": "3. 設定斜度",
        "calib.btn.step4": "4. 確認套用",

        "fine.skew.title": "手動調整 - 歪斜",
        "fine.start.title": "手動調整 - 起點",
        "fine.step": "步長：",
        "fine.manual_value": "手動數值：",
        "fine.apply": "套用",
        "fine.reset": "重設",
        "fine.confirm_apply": "確認套用",
        "fine.hint": "拖曳預覽：水平=偏移（+右 / -左），垂直=斜度（+下 / -上）",
    },
}


class Resampler:
    def __init__(self, input_rate, output_rate):
        self.step = float(input_rate) / float(output_rate)
        self.input_index = 0.0
        self.buffer = np.zeros(0, dtype=np.float32)

    def put(self, samples):
        if samples is None or len(samples) == 0:
            return
        self.buffer = np.concatenate([self.buffer, samples.astype(np.float32, copy=False)])

    def get_output(self, max_samples):
        if len(self.buffer) < 2:
            return np.zeros(0, dtype=np.float32)
        produced = []
        while len(produced) < max_samples:
            base = int(self.input_index)
            if base + 1 >= len(self.buffer):
                break
            frac = self.input_index - base
            s1 = self.buffer[base]
            s2 = self.buffer[base + 1]
            produced.append(s1 + frac * (s2 - s1))
            self.input_index += self.step

        consumed = int(self.input_index)
        if consumed > 0:
            if consumed >= len(self.buffer):
                self.buffer = np.zeros(0, dtype=np.float32)
                self.input_index = 0.0
            else:
                self.buffer = self.buffer[consumed:]
                self.input_index -= consumed

        if not produced:
            return np.zeros(0, dtype=np.float32)
        return np.asarray(produced, dtype=np.float32)


class SyncSegment:
    def __init__(self, start_line, skew, offset):
        self.start_line = int(start_line)
        self.skew_factor = float(skew)
        self.global_x_offset = float(offset)


def apply_transform_to_snapshot(snapshot, skew, offset, offset_scale=1.0):
    h, w = snapshot.shape
    if h == 0 or w == 0:
        return snapshot
    idx = np.arange(w, dtype=np.float64)
    rows = np.arange(h, dtype=np.float64)
    total_shift = rows[:, None] * float(skew) + float(offset) * float(offset_scale)
    eff = ((total_shift % w) + w) % w
    src_x = idx[None, :] - eff
    src_x = ((src_x % w) + w) % w
    x1 = src_x.astype(np.int64)
    x2 = (x1 + 1) % w
    frac = np.clip(src_x - x1, 0.0, 1.0)
    src_f = snapshot.astype(np.float32)
    row_idx = np.arange(h)[:, None]
    a = src_f[row_idx, x1]
    b = src_f[row_idx, x2]
    out = a * (1.0 - frac) + b * frac
    return np.clip(out, 0.0, 255.0).astype(np.uint8)


def parse_version(v):
    try:
        parts = re.findall(r"\d+", str(v))
        return tuple(int(p) for p in parts) if parts else (0,)
    except Exception:
        return (0,)


class FaxDecoderApp:
    def __init__(self):
        self.language = "zh_CN"
        self._load_settings()

        self.image_width = 2000
        self.seconds_per_line = 0.5
        self.contrast = 1.1
        self.is_binary_mode = False
        self.spectrum_decay_rate = 0.6

        self.is_decoding = False
        self.audio_stream = None
        self.pyaudio_instance = None
        self.process_thread = None
        self.current_real_sample_rate = SAMPLE_RATE_LIVE

        self.is_recording_audio = False
        self.record_stream = None
        self.record_pyaudio = None
        self.record_thread = None
        self.record_temp_path = None
        self.record_window = None

        self.current_line_index = 0
        self.processed_lines_data = []
        self.image_array = np.zeros((MAX_LINES, self.image_width), dtype=np.uint8)

        self.skew_factor = 0.0
        self.global_x_offset = 0.0
        self.sync_segments = [SyncSegment(0, 0.0, 0.0)]
        self.temp_skew = 0.0
        self.temp_offset = 0.0
        self.temp_split_line = 0
        self.is_in_manual_mode = False

        self.calib_state = "NONE"
        self.calib_p1 = (100, 100)
        self.calib_p2 = (300, 300)
        self.calib_start_x = 500
        self.calib_split_y = 400

        self.active_calib_window = None

        self.latest_fft_data = np.zeros(FFT_SIZE // 2, dtype=np.float32)
        self.display_fft_data = np.zeros(FFT_SIZE // 2, dtype=np.float32)

        self.selected_audio_device = None
        self.selected_file = None

        self.view_scale = 1.0
        self.view_offset_x = 0.0
        self.view_offset_y = 0.0
        self.drag_start = None
        self.img_display_x = 0.0
        self.img_display_y = 0.0
        self.img_display_scale = 1.0
        self._tk_image = None
        self.image_dirty = True

        self.file_pick_index = None
        self.file_decode_index = None

        self.data_lock = threading.Lock()
        self._closing = False
        self.file_decode_thread = None

        self.setup_ui()
        self.update_image_loop()
        self.update_spectrum_loop()

    def t(self, key, **kwargs):
        lang_dict = TRANSLATIONS.get(self.language, {})
        text = lang_dict.get(key)
        if text is None:
            text = TRANSLATIONS.get("zh_CN", {}).get(key, key)
        if kwargs:
            try:
                return text.format(**kwargs)
            except Exception:
                return text
        return text

    def _load_settings(self):
        cfg = configparser.ConfigParser()
        try:
            if os.path.exists(SETTINGS_FILE):
                cfg.read(SETTINGS_FILE, encoding="utf-8")
                if cfg.has_option("general", "language"):
                    lang = cfg.get("general", "language")
                    if lang in [code for code, _ in LANGUAGES]:
                        self.language = lang
        except Exception as e:
            print("load_settings err:", e)

    def _save_settings(self):
        try:
            os.makedirs(SETTINGS_DIR, exist_ok=True)
            cfg = configparser.ConfigParser()
            if os.path.exists(SETTINGS_FILE):
                try:
                    cfg.read(SETTINGS_FILE, encoding="utf-8")
                except Exception:
                    pass
            if not cfg.has_section("general"):
                cfg.add_section("general")
            cfg.set("general", "language", self.language)
            if not cfg.has_section("meta"):
                cfg.add_section("meta")
            cfg.set("meta", "version", APP_VERSION)
            cfg.set("meta", "last_saved",
                    datetime.now().isoformat(timespec="seconds"))
            with open(SETTINGS_FILE, "w", encoding="utf-8") as f:
                cfg.write(f)
        except Exception as e:
            print("save_settings err:", e)

    def set_language(self, lang):
        if lang == self.language:
            return
        if lang not in [code for code, _ in LANGUAGES]:
            return

        try:
            self.lang_var.set(self.language)
        except Exception:
            pass

        lang_name = next((name for code, name in LANGUAGES if code == lang), lang)

        dlg = tk.Toplevel(self.root)
        dlg.title(self.t("dlg.restart.title"))
        dlg.geometry("440x240")
        dlg.transient(self.root)
        dlg.grab_set()
        dlg.configure(bg="gray25")
        dlg.resizable(False, False)

        tk.Label(dlg, text=self.t("dlg.restart.title"),
                 bg="gray25", fg="#FFEB3B",
                 font=("Arial", 13, "bold")).pack(pady=(20, 10))
        tk.Label(dlg, text=self.t("dlg.restart.body", lang=lang_name),
                 bg="gray25", fg="white", font=("Arial", 10),
                 justify=tk.CENTER, wraplength=380).pack(pady=10)

        btn_row = tk.Frame(dlg, bg="gray25")
        btn_row.pack(pady=16)

        def do_confirm():
            self.language = lang
            self._save_settings()
            try:
                dlg.destroy()
            except Exception:
                pass
            self.on_close()

        def do_cancel():
            try:
                dlg.destroy()
            except Exception:
                pass

        tk.Button(btn_row, text=self.t("dlg.btn.ok"), width=12,
                  bg="#1565C0", fg="white",
                  activebackground="#0D47A1", activeforeground="white",
                  command=do_confirm).pack(side=tk.LEFT, padx=8)
        tk.Button(btn_row, text=self.t("dlg.btn.cancel"), width=12,
                  bg="#757575", fg="white",
                  activebackground="#616161", activeforeground="white",
                  command=do_cancel).pack(side=tk.LEFT, padx=8)

        dlg.bind("<Escape>", lambda e: do_cancel())

    def _build_menubar(self):
        menubar = Menu(self.root)

        # ---------- 文件菜单 ----------
        file_menu = Menu(menubar, tearoff=0)
        file_menu.add_command(label=self.t("menu.file.pick"),
                              command=self.pick_audio_file)
        file_menu.add_command(label=self.t("menu.file.decode"),
                              command=self.start_file_decoding)
        file_menu.add_separator()
        file_menu.add_command(label=self.t("menu.file.save"),
                              command=self.save_image)
        file_menu.add_separator()
        file_menu.add_command(label=self.t("menu.file.exit"),
                              command=self.on_close)

        # 用 label 反查索引（最稳健）
        self.file_pick_index = file_menu.index(self.t("menu.file.pick"))
        self.file_decode_index = file_menu.index(self.t("menu.file.decode"))

        menubar.add_cascade(label=self.t("menu.file"), menu=file_menu)
        self.file_menu = file_menu

        # ---------- 设置菜单 ----------
        settings_menu = Menu(menubar, tearoff=0)
        settings_menu.add_command(label=self.t("menu.settings.spectrum_decay"),
                                  command=self.input_spectrum_decay)
        settings_menu.add_command(label=self.t("menu.settings.width"),
                                  command=self.input_custom_width)
        settings_menu.add_command(label=self.t("menu.settings.contrast"),
                                  command=self.show_contrast_dialog)
        settings_menu.add_separator()

        # 语言子菜单
        self.lang_var = tk.StringVar(value=self.language)
        lang_menu = Menu(settings_menu, tearoff=0)
        for code, name in LANGUAGES:
            state = "disabled" if code == self.language else "normal"
            lang_menu.add_radiobutton(
                label=name,
                variable=self.lang_var,
                value=code,
                command=lambda c=code: self.set_language(c),
                state=state,
            )
        settings_menu.add_cascade(label=self.t("menu.settings.language"),
                                  menu=lang_menu)

        settings_menu.add_command(label=self.t("menu.settings.skew"),
                                  command=self.start_skew_calibration)
        settings_menu.add_command(label=self.t("menu.settings.start"),
                                  command=self.start_start_point_calibration)
        settings_menu.add_command(label=self.t("menu.settings.rpm"),
                                  command=self.show_rpm_menu)
        settings_menu.add_separator()
        settings_menu.add_command(label=self.t("menu.settings.manual"),
                                  command=self.show_manual_menu)

        # 反查索引
        self.menu_skew_index = settings_menu.index(self.t("menu.settings.skew"))
        self.menu_start_index = settings_menu.index(self.t("menu.settings.start"))
        self.menu_rpm_index = settings_menu.index(self.t("menu.settings.rpm"))
        self.menu_manual_index = settings_menu.index(self.t("menu.settings.manual"))

        menubar.add_cascade(label=self.t("menu.settings"), menu=settings_menu)
        self.settings_menu = settings_menu

        # ---------- 驱动菜单 ----------
        drivers_menu = Menu(menubar, tearoff=0)
        drivers_menu.add_command(label=self.t("menu.drivers.reset"),
                                 command=self.reset_audio_device)
        drivers_menu.add_command(label=self.t("menu.drivers.settings"),
                                 command=self.select_audio_device)
        menubar.add_cascade(label=self.t("menu.drivers"), menu=drivers_menu)

        # ---------- 录制菜单 ----------
        record_menu = Menu(menubar, tearoff=0)
        record_menu.add_command(label=self.t("menu.record.open"),
                                command=self.open_record_window)
        menubar.add_cascade(label=self.t("menu.record"), menu=record_menu)

        # ---------- 关于菜单 ----------
        about_menu = Menu(menubar, tearoff=0)
        about_menu.add_command(label=self.t("menu.about.about"),
                               command=self.show_about)
        about_menu.add_command(label=self.t("menu.about.check_update"),
                               command=self.check_update)
        menubar.add_cascade(label=self.t("menu.about"), menu=about_menu)

        self.menubar = menubar
        self.root.config(menu=menubar)

        if self.active_calib_window is not None:
            self.lock_calib_menus()

    def setup_ui(self):
        self.root = tk.Tk()
        self.root.title(self.t("app.title"))
        self.root.geometry("1400x820")
        self.root.configure(bg="black")

        self._build_menubar()

        main = tk.Frame(self.root, bg="black")
        main.pack(fill=tk.BOTH, expand=True)

        self.left_frame = tk.Frame(main, bg="black")
        self.left_frame.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        self.image_canvas = tk.Canvas(self.left_frame, bg="black", highlightthickness=0)
        self.image_canvas.pack(fill=tk.BOTH, expand=True)

        self.calib_bar = tk.Frame(self.left_frame, bg="gray20", height=44)
        self.calib_label = tk.Label(self.calib_bar, text="", bg="gray20", fg="white")
        self.calib_label.pack(side=tk.LEFT, padx=10)
        self.calib_btns_frame = tk.Frame(self.calib_bar, bg="gray20")
        self.calib_btns_frame.pack(side=tk.RIGHT, padx=10)

        right = tk.Frame(main, bg="gray25", width=340)
        right.pack(side=tk.RIGHT, fill=tk.Y)
        right.pack_propagate(False)

        top_btn_row = tk.Frame(right, bg="gray25")
        top_btn_row.pack(fill=tk.X, padx=6, pady=(8, 4))

        self.btn_start = tk.Button(
            top_btn_row, text=self.t("btn.start"), command=self.start_decoding,
            bg="#2E7D32", fg="white", activebackground="#1B5E20",
            activeforeground="white", font=("Arial", 10, "bold"),
            height=3, relief=tk.RAISED, bd=2
        )
        self.btn_start.pack(side=tk.LEFT, fill=tk.BOTH, expand=True, padx=2)

        self.btn_stop = tk.Button(
            top_btn_row, text=self.t("btn.stop"), command=self.stop_decoding,
            bg="#C62828", fg="white", activebackground="#8E0000",
            activeforeground="white", font=("Arial", 10, "bold"),
            height=3, relief=tk.RAISED, bd=2
        )
        self.btn_stop.pack(side=tk.LEFT, fill=tk.BOTH, expand=True, padx=2)

        self.btn_clear = tk.Button(
            top_btn_row, text=self.t("btn.clear"), command=self.clear_all,
            bg="#EF6C00", fg="white", activebackground="#BF360C",
            activeforeground="white", font=("Arial", 10, "bold"),
            height=3, relief=tk.RAISED, bd=2
        )
        self.btn_clear.pack(side=tk.LEFT, fill=tk.BOTH, expand=True, padx=2)

        tk.Label(right, text=self.t("label.spectrum"),
                 bg="gray25", fg="lightgray", font=("Arial", 9)).pack(
            anchor="w", padx=8, pady=(10, 0))
        self.spectrum_canvas = tk.Canvas(right, bg="black", height=200, highlightthickness=0)
        self.spectrum_canvas.pack(fill=tk.X, padx=8, pady=4)

        status_frame = tk.Frame(right, bg="gray25")
        status_frame.pack(fill=tk.X, padx=8, pady=(8, 4))
        tk.Label(status_frame, text=self.t("label.status"),
                 bg="gray25", fg="lightgray", font=("Arial", 9)).pack(anchor="w")
        self.status_label = tk.Label(
            status_frame, text=self.t("status.ready"), bg="gray25", fg="white",
            anchor="w", justify=tk.LEFT, wraplength=320, font=("Arial", 9)
        )
        self.status_label.pack(fill=tk.X)

        self.image_canvas.bind("<ButtonPress-1>", self.on_mouse_down)
        self.image_canvas.bind("<B1-Motion>", self.on_mouse_move)
        self.image_canvas.bind("<ButtonRelease-1>", self.on_mouse_up)
        self.image_canvas.bind("<MouseWheel>", self.on_mouse_wheel)
        self.image_canvas.bind("<Button-4>", lambda e: self.on_mouse_wheel(e, 1))
        self.image_canvas.bind("<Button-5>", lambda e: self.on_mouse_wheel(e, -1))
        self.image_canvas.bind("<Configure>", lambda e: self.mark_image_dirty())

    def set_status(self, text):
        if self._closing:
            return
        try:
            self.status_label.config(text=text)
        except Exception:
            pass

    def _set_calib_menu_state(self, state):
        for idx in (self.menu_skew_index, self.menu_start_index,
                    self.menu_rpm_index, self.menu_manual_index):
            if idx is None:
                continue
            try:
                self.settings_menu.entryconfig(idx, state=state)
            except Exception:
                pass

    def _set_file_menu_state(self, state):
        for idx in (self.file_pick_index, self.file_decode_index):
            if idx is None:
                continue
            try:
                self.file_menu.entryconfig(idx, state=state)
            except Exception:
                pass

    def _set_top_buttons_state(self, state):
        for btn in (self.btn_start, self.btn_clear):
            try:
                btn.config(state=state)
            except Exception:
                pass

    def lock_calib_menus(self):
        self._set_calib_menu_state("disabled")
        self._set_file_menu_state("disabled")
        self._set_top_buttons_state("disabled")

    def unlock_calib_menus(self):
        self._set_calib_menu_state("normal")
        self._set_file_menu_state("normal")
        self._set_top_buttons_state("normal")

    def _set_manual_menu_state(self, state):
        try:
            self.settings_menu.entryconfig(self.menu_manual_index, state=state)
        except Exception:
            pass

    def show_about(self):
        dlg = tk.Toplevel(self.root)
        dlg.title(self.t("dlg.about.title"))
        dlg.geometry("380x240")
        dlg.transient(self.root)
        dlg.grab_set()
        dlg.configure(bg="gray25")
        dlg.resizable(False, False)

        tk.Label(dlg, text="FAX Decoder for PC", bg="gray25", fg="white",
                 font=("Arial", 14, "bold")).pack(pady=(20, 6))
        tk.Label(dlg, text=f'{self.t("dlg.about.author")}: {APP_AUTHOR}',
                 bg="gray25", fg="#FFEB3B", font=("Arial", 11)).pack(pady=4)
        tk.Label(dlg, text=f'{self.t("dlg.about.version")}: {APP_VERSION}',
                 bg="gray25", fg="lightgray", font=("Arial", 11)).pack(pady=4)
        tk.Label(dlg, text="https://github.com/78-HAM/FAX-Decoder-for-PC",
                 bg="gray25", fg="#4FC3F7", font=("Arial", 9)).pack(pady=(8, 4))

        tk.Button(dlg, text=self.t("dlg.btn.ok"), width=12, command=dlg.destroy,
                  bg="#546E7A", fg="white").pack(pady=12)

    def check_update(self):
        def fetch_json(url):
            req = urllib.request.Request(
                url,
                headers={
                    "User-Agent": "Mozilla/5.0 FAX-Decoder-Updater",
                    "Accept": "application/vnd.github+json",
                    "Cache-Control": "no-cache",
                }
            )
            with urllib.request.urlopen(req, timeout=8) as resp:
                return json.loads(resp.read().decode("utf-8"))

        def worker():
            latest_version = None
            latest_url = GITHUB_RELEASES_PAGE
            any_success = False

            try:
                releases = fetch_json(GITHUB_RELEASES_API)
                any_success = True
                if isinstance(releases, list):
                    for rel in releases:
                        tag = (rel.get("tag_name") or "").strip()
                        if not tag:
                            tag = (rel.get("name") or "").strip()
                        if not tag:
                            continue
                        if latest_version is None or parse_version(tag) > parse_version(latest_version):
                            latest_version = tag
                            latest_url = rel.get("html_url", GITHUB_RELEASES_PAGE)
            except Exception:
                pass

            if latest_version is None:
                try:
                    tags = fetch_json(GITHUB_TAGS_API)
                    any_success = True
                    if isinstance(tags, list):
                        for t in tags:
                            name = (t.get("name") or "").strip()
                            if not name:
                                continue
                            if latest_version is None or parse_version(name) > parse_version(latest_version):
                                latest_version = name
                                latest_url = f"{GITHUB_RELEASES_PAGE}/tag/{name}"
                except Exception:
                    pass

            if self._closing:
                return

            if not any_success or latest_version is None:
                self.root.after(0, lambda: messagebox.showerror(
                    self.t("dlg.update.failed_title"),
                    self.t("dlg.update.failed_body")))
                return

            cur = parse_version(APP_VERSION)
            new = parse_version(latest_version)

            if new > cur:
                def ask():
                    if self._closing:
                        return
                    msg = (f'{self.t("dlg.update.found_body")} {latest_version}\n'
                           f'{self.t("dlg.update.current_version")} {APP_VERSION}\n\n'
                           f'{self.t("dlg.update.go")}')
                    if messagebox.askyesno(self.t("dlg.update.found_title"), msg):
                        webbrowser.open(latest_url)
                self.root.after(0, ask)
            else:
                self.root.after(0, lambda: messagebox.showinfo(
                    self.t("dlg.update.latest_title"),
                    f'{self.t("dlg.update.latest_body")} ({APP_VERSION})'))

        threading.Thread(target=worker, daemon=True).start()

    def _experimental_warning(self, title, on_confirm):
        dlg = tk.Toplevel(self.root)
        dlg.title(self.t("dlg.experimental.title"))
        dlg.geometry("420x200")
        dlg.transient(self.root)
        dlg.grab_set()
        dlg.configure(bg="gray25")
        dlg.resizable(False, False)

        tk.Label(dlg, text=title, bg="gray25", fg="#FF5252",
                 font=("Arial", 13, "bold")).pack(pady=(18, 6))
        tk.Label(dlg, text=self.t("dlg.experimental.body"),
                 bg="gray25", fg="white", font=("Arial", 10),
                 justify=tk.CENTER).pack(pady=6)

        btn_row = tk.Frame(dlg, bg="gray25")
        btn_row.pack(pady=16)

        def confirm():
            dlg.destroy()
            self.root.after(60, on_confirm)

        tk.Button(btn_row, text=self.t("dlg.btn.cancel"), width=12,
                  bg="#1565C0", fg="white",
                  activebackground="#0D47A1", activeforeground="white",
                  command=dlg.destroy).pack(side=tk.LEFT, padx=8)
        tk.Button(btn_row, text=self.t("dlg.btn.ok"), width=12,
                  bg="#757575", fg="white",
                  activebackground="#616161", activeforeground="white",
                  command=confirm).pack(side=tk.LEFT, padx=8)

    def reset_audio_device(self):
        self.selected_audio_device = None
        self.set_status(self.t("status.audio_device_reset"))

    def select_audio_device(self):
        try:
            p = pyaudio.PyAudio()
        except Exception as e:
            messagebox.showerror(self.t("app.title"),
                                 f'{self.t("msg.pyaudio_failed")}: {e}')
            return
        devices = []
        for i in range(p.get_device_count()):
            try:
                info = p.get_device_info_by_index(i)
            except Exception:
                continue
            if info.get("maxInputChannels", 0) > 0:
                devices.append((i, info.get("name", f"device {i}")))
        p.terminate()
        if not devices:
            messagebox.showwarning(self.t("app.title"), self.t("msg.no_input_device"))
            return

        dlg = tk.Toplevel(self.root)
        dlg.title(self.t("dlg.pick_device.title"))
        dlg.geometry("500x400")
        dlg.transient(self.root)
        dlg.grab_set()
        dlg.configure(bg="gray25")

        tk.Label(dlg, text=self.t("dlg.pick_device.hint"),
                 bg="gray25", fg="white", wraplength=460, justify=tk.LEFT).pack(
            padx=10, pady=8, anchor="w")

        list_frame = tk.Frame(dlg, bg="gray25")
        list_frame.pack(fill=tk.BOTH, expand=True, padx=10, pady=4)
        scrollbar = tk.Scrollbar(list_frame)
        scrollbar.pack(side=tk.RIGHT, fill=tk.Y)
        lb = tk.Listbox(list_frame, yscrollcommand=scrollbar.set)
        lb.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        scrollbar.config(command=lb.yview)

        for idx, name in devices:
            lb.insert(tk.END, f"[{idx}] {name}")

        if self.selected_audio_device is not None:
            for k, (idx, _) in enumerate(devices):
                if idx == self.selected_audio_device:
                    lb.selection_set(k)
                    lb.see(k)
                    break

        def ok():
            sel = lb.curselection()
            if sel:
                self.selected_audio_device = devices[sel[0]][0]
                self.set_status(f'{self.t("status.audio_device_selected")}: {devices[sel[0]][1]}')
            dlg.destroy()

        btnf = tk.Frame(dlg, bg="gray25")
        btnf.pack(pady=8)
        tk.Button(btnf, text=self.t("dlg.btn.ok"), width=10, command=ok).pack(side=tk.LEFT, padx=4)
        tk.Button(btnf, text=self.t("dlg.btn.cancel"), width=10,
                  command=dlg.destroy).pack(side=tk.LEFT, padx=4)

    def input_spectrum_decay(self):
        def do_input():
            v = simpledialog.askfloat(self.t("dlg.spectrum_decay.title"),
                                      self.t("dlg.spectrum_decay.hint"),
                                      initialvalue=self.spectrum_decay_rate,
                                      parent=self.root)
            if v is not None and 0.05 < v < 1.0:
                self.spectrum_decay_rate = v
                self.set_status(f'{self.t("status.spectrum_decay")}: {v}')

        self._experimental_warning(self.t("dlg.spectrum_decay.title"), do_input)

    def input_custom_width(self):
        def do_input():
            v = simpledialog.askinteger(self.t("dlg.width.title"),
                                        self.t("dlg.width.hint"),
                                        initialvalue=self.image_width,
                                        parent=self.root)
            if v and v > 0:
                self.image_width = v
                self.reinit_bitmap()

        self._experimental_warning(self.t("dlg.width.title"), do_input)

    def show_contrast_dialog(self):
        dlg = tk.Toplevel(self.root)
        dlg.title(self.t("dlg.contrast.title"))
        dlg.geometry("400x220")
        dlg.transient(self.root)
        dlg.grab_set()
        dlg.configure(bg="gray25")

        tk.Label(dlg, text=self.t("dlg.contrast.hint"),
                 bg="gray25", fg="white").pack(pady=6, anchor="w", padx=10)
        var = tk.IntVar(value=int(self.contrast * 100))
        tk.Scale(dlg, from_=0, to=500, orient=tk.HORIZONTAL, variable=var,
                 length=360, label="%").pack(padx=10)
        binary_var = tk.BooleanVar(value=self.is_binary_mode)
        tk.Checkbutton(dlg, text=self.t("dlg.contrast.binary"), variable=binary_var,
                       bg="gray25", fg="white", selectcolor="gray40").pack(pady=4)

        def apply():
            self.contrast = var.get() / 100.0
            self.is_binary_mode = bool(binary_var.get())
            dlg.destroy()
            self.redraw_all_history()

        btnf = tk.Frame(dlg, bg="gray25")
        btnf.pack(pady=6)
        tk.Button(btnf, text=self.t("dlg.btn.ok"), width=10,
                  command=apply).pack(side=tk.LEFT, padx=4)
        tk.Button(btnf, text=self.t("dlg.btn.cancel"), width=10,
                  command=dlg.destroy).pack(side=tk.LEFT, padx=4)

    def show_rpm_menu(self):
        dlg = tk.Toplevel(self.root)
        dlg.title(self.t("dlg.rpm.title"))
        dlg.geometry("240x320")
        dlg.transient(self.root)
        dlg.grab_set()
        dlg.configure(bg="gray25")

        options = [("30 RPM", 2.0), ("60 RPM", 1.0), ("120 RPM", 0.5),
                   ("240 RPM", 0.25), ("480 RPM", 0.125)]

        def choose(s):
            self.seconds_per_line = s
            self.clear_all()
            self.set_status(f'{self.t("status.per_line")}: {s} {self.t("status.seconds")}')
            dlg.destroy()

        for label, val in options:
            tk.Button(dlg, text=label, height=2,
                      command=lambda v=val: choose(v)).pack(fill=tk.X, padx=10, pady=3)
        tk.Button(dlg, text=self.t("dlg.btn.back"),
                  command=dlg.destroy).pack(fill=tk.X, padx=10, pady=10)

    def show_calib_bar(self, text, buttons):
        for w in self.calib_btns_frame.winfo_children():
            w.destroy()
        for (label, cb) in buttons:
            tk.Button(self.calib_btns_frame, text=label, command=cb,
                      bg="gray40", fg="white", padx=8).pack(side=tk.LEFT, padx=3)
        self.calib_label.config(text=text)
        self.calib_bar.pack(fill=tk.X, side=tk.BOTTOM)
        self.mark_image_dirty()

    def hide_calib_bar(self):
        self.calib_bar.pack_forget()

    def start_skew_calibration(self):
        if self.current_line_index == 0:
            messagebox.showinfo(self.t("app.title"), self.t("msg.no_image_data"))
            return
        if self.active_calib_window is not None:
            return

        self.lock_calib_menus()
        self.is_in_manual_mode = False
        self.calib_state = "SKEW_P1"
        self.calib_p1 = (self.image_canvas.winfo_width() // 2,
                         self.image_canvas.winfo_height() // 3)
        self.calib_p2 = self.calib_p1
        self.active_calib_window = "skew"
        self.show_calib_bar(
            self.t("calib.skew.p1"),
            [(self.t("calib.btn.fine_tune"), lambda: self.open_fine_tune("skew")),
             (self.t("calib.btn.confirm_p1"), self._skew_next),
             (self.t("calib.btn.cancel"), self.exit_calibration)])

    def _skew_next(self):
        self.calib_state = "SKEW_P2"
        if self.calib_p2 == self.calib_p1:
            self.calib_p2 = (self.calib_p1[0] + 200, self.calib_p1[1] + 200)
        self.show_calib_bar(
            self.t("calib.skew.p2"),
            [(self.t("calib.btn.fine_tune"), lambda: self.open_fine_tune("skew")),
             (self.t("calib.btn.apply"), self.apply_skew_calibration),
             (self.t("calib.btn.back"), self.start_skew_calibration)])

    def start_start_point_calibration(self):
        if self.current_line_index == 0:
            messagebox.showinfo(self.t("app.title"), self.t("msg.no_image_data"))
            return
        if self.active_calib_window is not None:
            return

        self.lock_calib_menus()
        self.is_in_manual_mode = False
        self.calib_state = "START_POINT"
        self.calib_start_x = self.image_canvas.winfo_width() // 2
        self.active_calib_window = "start"
        self.show_calib_bar(
            self.t("calib.start.hint"),
            [(self.t("calib.btn.fine_tune"), lambda: self.open_fine_tune("start")),
             (self.t("calib.btn.confirm_pos"), self.apply_start_point_calibration),
             (self.t("calib.btn.cancel"), self.exit_calibration)])

    def exit_calibration(self):
        self.calib_state = "NONE"
        self.is_in_manual_mode = False
        self.active_calib_window = None
        self.hide_calib_bar()
        self.unlock_calib_menus()
        self._set_manual_menu_state("disabled" if self.is_decoding else "normal")
        self.mark_image_dirty()
        self.set_status(self.t("status.exited_calib"))

    def apply_start_point_calibration(self):
        x_img = float((self.calib_start_x - self.img_display_x) /
                      max(self.img_display_scale, 1e-6))
        if self.is_in_manual_mode:
            self.temp_offset = (self.temp_offset - x_img) % self.image_width
            if self.temp_offset < 0:
                self.temp_offset += self.image_width
            self.redraw_preview_with_temp()
            self.show_manual_menu()
        else:
            self.global_x_offset = (self.global_x_offset - x_img) % self.image_width
            if self.global_x_offset < 0:
                self.global_x_offset += self.image_width
            self.redraw_all_history()
            self.exit_calibration()

    def apply_skew_calibration(self):
        x1 = (self.calib_p1[0] - self.img_display_x) / max(self.img_display_scale, 1e-6)
        y1 = (self.calib_p1[1] - self.img_display_y) / max(self.img_display_scale, 1e-6)
        x2 = (self.calib_p2[0] - self.img_display_x) / max(self.img_display_scale, 1e-6)
        y2 = (self.calib_p2[1] - self.img_display_y) / max(self.img_display_scale, 1e-6)
        dy = y2 - y1
        if abs(dy) < 1.0:
            dy = 1.0
        delta_skew = -(x2 - x1) / dy
        if self.is_in_manual_mode:
            old_skew = self.temp_skew
            self.temp_skew += delta_skew
            self.temp_offset = (self.temp_offset + y1 * (old_skew - self.temp_skew)) % self.image_width
            if self.temp_offset < 0:
                self.temp_offset += self.image_width
            self.redraw_preview_with_temp()
            self.show_manual_menu()
        else:
            old_skew = self.skew_factor
            self.skew_factor += delta_skew
            self.global_x_offset = (self.global_x_offset +
                                    y1 * (old_skew - self.skew_factor)) % self.image_width
            if self.global_x_offset < 0:
                self.global_x_offset += self.image_width
            self.redraw_all_history()
            self.exit_calibration()

    def build_fine_tune_snapshot(self, max_pixels=1_000_000):
        n_rows = len(self.processed_lines_data)
        if n_rows == 0:
            return None, 1.0
        w = self.image_width
        if n_rows * w > max_pixels:
            ratio = (max_pixels / float(n_rows * w)) ** 0.5
        else:
            ratio = 1.0
        w_small = max(1, int(round(w * ratio)))
        h_small = max(1, int(round(n_rows * ratio)))
        col_pos_orig = np.arange(w_small, dtype=np.float64) / ratio
        arr = np.zeros((h_small, w_small), dtype=np.uint8)
        for i in range(h_small):
            orig_row_f = i / ratio
            orig_row = int(round(orig_row_f))
            if orig_row >= n_rows:
                orig_row = n_rows - 1
            data = self.processed_lines_data[orig_row]
            if self.is_in_manual_mode and orig_row >= self.temp_split_line:
                skew = float(self.temp_skew)
                offset = float(self.temp_offset)
            else:
                seg = self.get_segment_for_line(orig_row)
                skew = seg.skew_factor
                offset = seg.global_x_offset
            total_shift = orig_row_f * skew + offset
            eff = ((total_shift % w) + w) % w
            src_x = col_pos_orig - eff
            src_x = ((src_x % w) + w) % w
            x1 = src_x.astype(np.int64) % w
            x2 = (x1 + 1) % w
            frac = np.clip(src_x - x1, 0.0, 1.0)
            val = data[x1] * (1.0 - frac) + data[x2] * frac
            arr[i] = np.clip(val * 255.0, 0.0, 255.0).astype(np.uint8)
        return arr, ratio

    def open_fine_tune(self, mode):
        if len(self.processed_lines_data) == 0:
            messagebox.showinfo(self.t("app.title"), self.t("msg.no_image_data"))
            return
        snapshot, ratio = self.build_fine_tune_snapshot()
        if snapshot is None or snapshot.size == 0:
            messagebox.showinfo(self.t("app.title"), self.t("msg.empty_image"))
            return

        if self.is_in_manual_mode:
            base_skew = float(self.temp_skew)
            base_offset = float(self.temp_offset)
        else:
            base_skew = float(self.skew_factor)
            base_offset = float(self.global_x_offset)

        win = tk.Toplevel(self.root)
        win.title(self.t("fine.skew.title") if mode == "skew" else self.t("fine.start.title"))
        win.geometry("1100x850")
        win.configure(bg="gray25")
        win.transient(self.root)

        state = {"skew": base_skew, "offset": base_offset}
        key = "skew" if mode == "skew" else "offset"

        canvas = tk.Canvas(win, bg="black", highlightthickness=0)
        canvas.pack(fill=tk.BOTH, expand=True, padx=8, pady=8)

        info_label = tk.Label(win, text="", bg="gray25", fg="#FFEB3B",
                              font=("Consolas", 11), justify=tk.LEFT, anchor="w")
        info_label.pack(fill=tk.X, padx=10, pady=(0, 4))

        photo_ref = [None]
        render_job = [None]
        ctx = {"rendered_w": 1, "rendered_h": 1, "scale": 1.0}

        def update_info():
            info_label.config(
                text=(f"skew = {state['skew']:.5f}    offset = {state['offset']:.2f}\n"
                      f"{self.t('fine.hint')}")
            )

        def render():
            render_job[0] = None
            if not win.winfo_exists():
                return
            cw = canvas.winfo_width()
            ch = canvas.winfo_height()
            if cw < 30 or ch < 30:
                render_job[0] = win.after(100, render)
                return
            try:
                delta_skew = state["skew"] - base_skew
                delta_offset = state["offset"] - base_offset
                transformed = apply_transform_to_snapshot(
                    snapshot, delta_skew, delta_offset, offset_scale=ratio)
            except Exception as e:
                print("transform err:", e)
                return
            pil = Image.fromarray(transformed, mode="L")
            sw, sh = pil.size
            if sw <= 0 or sh <= 0:
                return
            scale = min(cw / sw, ch / sh) * 0.98
            nw = max(1, int(sw * scale))
            nh = max(1, int(sh * scale))
            img = pil.resize((nw, nh), Image.NEAREST)
            photo_ref[0] = ImageTk.PhotoImage(img)
            canvas.delete("all")
            canvas.create_image(cw // 2, ch // 2, image=photo_ref[0], anchor=tk.CENTER)
            ctx["rendered_w"] = nw
            ctx["rendered_h"] = nh
            ctx["scale"] = scale
            update_info()

        def schedule_render(delay=40):
            if render_job[0] is not None:
                try:
                    win.after_cancel(render_job[0])
                except Exception:
                    pass
            render_job[0] = win.after(delay, render)

        drag = {"x": 0, "y": 0, "skew0": 0.0, "offset0": 0.0, "active": False}

        def on_press(e):
            drag["x"] = e.x
            drag["y"] = e.y
            drag["skew0"] = state["skew"]
            drag["offset0"] = state["offset"]
            drag["active"] = True

        def on_motion(e):
            if not drag["active"]:
                return
            dx = e.x - drag["x"]
            dy = e.y - drag["y"]
            rw = max(1, ctx["rendered_w"])
            orig_per_canvas_px = float(self.image_width) / rw
            state["offset"] = drag["offset0"] + dx * orig_per_canvas_px
            if mode == "skew":
                rh = max(1, ctx["rendered_h"])
                state["skew"] = drag["skew0"] + dy * (1.0 / rh)
            schedule_render(25)

        def on_release(e):
            drag["active"] = False

        canvas.bind("<ButtonPress-1>", on_press)
        canvas.bind("<B1-Motion>", on_motion)
        canvas.bind("<ButtonRelease-1>", on_release)
        canvas.bind("<Configure>", lambda e: schedule_render(120))

        ctrl = tk.Frame(win, bg="gray25")
        ctrl.pack(fill=tk.X, padx=10, pady=8)

        row1 = tk.Frame(ctrl, bg="gray25")
        row1.pack(fill=tk.X, pady=4)

        default_step = 0.01 if mode == "skew" else 10.0
        step_var = tk.DoubleVar(value=default_step)

        def do_step(sign):
            try:
                v = float(step_var.get())
            except Exception:
                return
            state[key] += sign * v
            schedule_render()

        tk.Button(row1, text="◀", width=5, height=2,
                  font=("Arial", 14, "bold"), bg="#1565C0", fg="white",
                  command=lambda: do_step(-1)).pack(side=tk.LEFT, padx=4)

        mid = tk.Frame(row1, bg="gray25")
        mid.pack(side=tk.LEFT, padx=8)
        tk.Label(mid, text=self.t("fine.step"), bg="gray25", fg="white",
                 font=("Arial", 11)).pack(side=tk.LEFT)
        tk.Entry(mid, textvariable=step_var, width=10,
                 font=("Arial", 11)).pack(side=tk.LEFT)

        tk.Button(row1, text="▶", width=5, height=2,
                  font=("Arial", 14, "bold"), bg="#1565C0", fg="white",
                  command=lambda: do_step(1)).pack(side=tk.LEFT, padx=4)

        row2 = tk.Frame(ctrl, bg="gray25")
        row2.pack(fill=tk.X, pady=4)

        manual_var = tk.DoubleVar(value=0.0)
        tk.Label(row2, text=self.t("fine.manual_value"), bg="gray25", fg="white",
                 font=("Arial", 11)).pack(side=tk.LEFT)
        tk.Entry(row2, textvariable=manual_var, width=12,
                 font=("Arial", 11)).pack(side=tk.LEFT, padx=4)

        def apply_manual():
            try:
                v = float(manual_var.get())
            except Exception:
                return
            state[key] += v
            schedule_render()

        tk.Button(row2, text=self.t("fine.apply"), width=8, command=apply_manual,
                  bg="#546E7A", fg="white").pack(side=tk.LEFT, padx=6)

        row3 = tk.Frame(ctrl, bg="gray25")
        row3.pack(fill=tk.X, pady=(12, 4))

        def reset_values():
            state["skew"] = base_skew
            state["offset"] = base_offset
            schedule_render()

        tk.Button(row3, text=self.t("fine.reset"), width=10, command=reset_values,
                  bg="#455A64", fg="white").pack(side=tk.LEFT, padx=4)

        def do_confirm():
            if self.is_in_manual_mode:
                if mode == "skew":
                    self.temp_skew = state["skew"]
                else:
                    v = state["offset"] % self.image_width
                    if v < 0:
                        v += self.image_width
                    self.temp_offset = v
                win.destroy()
                self.redraw_preview_with_temp()
            else:
                if mode == "skew":
                    self.skew_factor = state["skew"]
                else:
                    v = state["offset"] % self.image_width
                    if v < 0:
                        v += self.image_width
                    self.global_x_offset = v
                win.destroy()
                self.redraw_all_history()
            self.set_status(self.t("status.manual_applied"))

        tk.Button(row3, text=self.t("dlg.btn.cancel"), width=10, command=win.destroy,
                  bg="#616161", fg="white").pack(side=tk.RIGHT, padx=4)
        tk.Button(row3, text=self.t("fine.confirm_apply"), width=12, command=do_confirm,
                  bg="#009688", fg="white",
                  font=("Arial", 10, "bold")).pack(side=tk.RIGHT, padx=4)

        win.bind("<Escape>", lambda e: win.destroy())

        update_info()
        win.after(80, render)
        win.after(300, render)
        win.after(700, render)

    def show_manual_menu(self):
        if self.current_line_index == 0:
            messagebox.showinfo(self.t("app.title"), self.t("msg.no_image_data"))
            return
        if self.is_decoding:
            messagebox.showwarning(self.t("app.title"), self.t("msg.manual_need_stop"))
            return
        if self.active_calib_window is not None and self.active_calib_window != "manual":
            return

        self.lock_calib_menus()
        self.is_in_manual_mode = True
        self.calib_state = "NONE"
        self.active_calib_window = "manual"
        self.show_calib_bar(
            self.t("calib.manual.hint"),
            [(self.t("calib.btn.step1"), self.start_split_line_selection),
             (self.t("calib.btn.step2"), self.start_multi_start_calib),
             (self.t("calib.btn.step3"), self.start_multi_skew_calib),
             (self.t("calib.btn.step4"), self.commit_manual_segment),
             (self.t("dlg.btn.back"), self.exit_calibration)])

    def start_split_line_selection(self):
        self.calib_state = "SPLIT_SELECT"
        self.calib_split_y = self.image_canvas.winfo_height() // 2
        self.show_calib_bar(self.t("calib.manual.split"),
                            [(self.t("calib.btn.cancel"), self.show_manual_menu)])

    def _finish_split_selection(self):
        y_img = (self.calib_split_y - self.img_display_y) / max(self.img_display_scale, 1e-6)
        self.temp_split_line = int(max(0, min(MAX_LINES - 1, round(y_img))))
        seg = self.get_segment_for_line(self.temp_split_line)
        self.temp_skew = seg.skew_factor
        self.temp_offset = seg.global_x_offset
        self.set_status(f'{self.t("status.split_line")}: {self.temp_split_line} {self.t("status.line")}')
        self.show_manual_menu()

    def start_multi_start_calib(self):
        self.calib_state = "START_POINT"
        self.calib_start_x = self.image_canvas.winfo_width() // 2
        self.show_calib_bar(
            self.t("calib.manual.start"),
            [(self.t("calib.btn.fine_tune"), lambda: self.open_fine_tune("start")),
             (self.t("calib.btn.confirm_pos"), self.apply_start_point_calibration),
             (self.t("calib.btn.cancel"), self.show_manual_menu)])

    def start_multi_skew_calib(self):
        self.calib_state = "SKEW_P1"
        self.calib_p1 = (self.image_canvas.winfo_width() // 2,
                         self.image_canvas.winfo_height() // 3)
        self.calib_p2 = self.calib_p1
        self.show_calib_bar(
            self.t("calib.manual.skew_p1"),
            [(self.t("calib.btn.fine_tune"), lambda: self.open_fine_tune("skew")),
             (self.t("calib.btn.confirm_p1"), self._manual_skew_next),
             (self.t("calib.btn.cancel"), self.show_manual_menu)])

    def _manual_skew_next(self):
        self.calib_state = "SKEW_P2"
        if self.calib_p2 == self.calib_p1:
            self.calib_p2 = (self.calib_p1[0] + 200, self.calib_p1[1] + 200)
        self.show_calib_bar(
            self.t("calib.manual.skew_p2"),
            [(self.t("calib.btn.fine_tune"), lambda: self.open_fine_tune("skew")),
             (self.t("calib.btn.apply"), self.apply_skew_calibration),
             (self.t("dlg.btn.back"), self.start_multi_skew_calib)])

    def commit_manual_segment(self):
        self.sync_segments = [s for s in self.sync_segments if s.start_line < self.temp_split_line]
        self.sync_segments.append(SyncSegment(self.temp_split_line,
                                              self.temp_skew, self.temp_offset))
        self.do_flatten()
        self.exit_calibration()

    def do_flatten(self):
        def worker():
            new_data = []
            for i, src_row in enumerate(self.processed_lines_data):
                seg = self.get_segment_for_line(i)
                skew = seg.skew_factor
                offset = seg.global_x_offset
                idx = np.arange(self.image_width, dtype=np.float64)
                total_shift = i * skew + offset
                eff = ((total_shift % self.image_width) + self.image_width) % self.image_width
                src_x = idx - eff
                src_x = ((src_x % self.image_width) + self.image_width) % self.image_width
                x1 = src_x.astype(np.int64)
                x2 = (x1 + 1) % self.image_width
                frac = np.clip(src_x - x1, 0.0, 1.0)
                new_row = src_row[x1] * (1.0 - frac) + src_row[x2] * frac
                new_data.append(new_row)
            if self._closing:
                return
            self.processed_lines_data = new_data
            self.skew_factor = 0.0
            self.global_x_offset = 0.0
            self.temp_skew = 0.0
            self.temp_offset = 0.0
            self.sync_segments = [SyncSegment(0, 0.0, 0.0)]
            self.redraw_all_history()
            if not self._closing:
                self.root.after(0, lambda: messagebox.showinfo(
                    self.t("app.title"), self.t("msg.calib_applied")))
        threading.Thread(target=worker, daemon=True).start()

    def on_mouse_down(self, event):
        if self.calib_state != "NONE":
            self._update_calib_point(event.x, event.y)
            self.mark_image_dirty()
            return
        self.drag_start = (event.x, event.y, self.view_offset_x, self.view_offset_y)

    def on_mouse_move(self, event):
        if self.calib_state != "NONE":
            self._update_calib_point(event.x, event.y)
            self.mark_image_dirty()
            return
        if self.drag_start is not None:
            dx = event.x - self.drag_start[0]
            dy = event.y - self.drag_start[1]
            self.view_offset_x = self.drag_start[2] + dx
            self.view_offset_y = self.drag_start[3] + dy
            self.mark_image_dirty()

    def on_mouse_up(self, event):
        if self.calib_state == "SPLIT_SELECT":
            self._finish_split_selection()
            return
        self.drag_start = None

    def _update_calib_point(self, x, y):
        if self.calib_state == "SKEW_P1":
            self.calib_p1 = (x, y)
        elif self.calib_state == "SKEW_P2":
            self.calib_p2 = (x, y)
        elif self.calib_state == "START_POINT":
            self.calib_start_x = x
        elif self.calib_state == "SPLIT_SELECT":
            self.calib_split_y = y

    def on_mouse_wheel(self, event, delta=None):
        if delta is None:
            delta = 1 if event.delta > 0 else -1
        factor = 1.1 ** delta
        self.view_scale = max(0.05, min(20.0, self.view_scale * factor))
        self.mark_image_dirty()

    def mark_image_dirty(self):
        self.image_dirty = True

    def update_image_loop(self):
        if self._closing:
            return
        if self.image_dirty:
            self.image_dirty = False
            try:
                self.redraw_image()
            except Exception as e:
                print("redraw error:", e)
        try:
            self.root.after(80, self.update_image_loop)
        except Exception:
            pass

    def redraw_image(self):
        self.image_canvas.delete("all")
        if self.current_line_index == 0:
            return
        with self.data_lock:
            img_data = self.image_array[:self.current_line_index, :].copy()
        pil_img = Image.fromarray(img_data, mode="L")
        canvas_w = max(1, self.image_canvas.winfo_width())
        canvas_h = max(1, self.image_canvas.winfo_height())
        src_w, src_h = pil_img.size
        if src_w <= 0 or src_h <= 0:
            return
        base_scale = min(canvas_w / src_w, canvas_h / src_h)
        scale = base_scale * self.view_scale
        new_w = max(1, int(src_w * scale))
        new_h = max(1, int(src_h * scale))
        max_dim = 3000
        if new_w > max_dim or new_h > max_dim:
            k = max_dim / max(new_w, new_h)
            new_w = max(1, int(new_w * k))
            new_h = max(1, int(new_h * k))
        render = pil_img.resize((new_w, new_h), Image.NEAREST)
        self._tk_image = ImageTk.PhotoImage(render)
        x = canvas_w // 2 - new_w // 2 + self.view_offset_x
        y = canvas_h // 2 - new_h // 2 + self.view_offset_y
        self.img_display_x = x
        self.img_display_y = y
        self.img_display_scale = (new_w / src_w)
        self.image_canvas.create_image(x, y, anchor=tk.NW, image=self._tk_image)
        self.draw_calibration_overlay()

    def draw_calibration_overlay(self):
        cs = self.calib_state
        if cs == "NONE" and not self.is_in_manual_mode:
            return
        W = max(1, self.image_canvas.winfo_width())
        H = max(1, self.image_canvas.winfo_height())
        if self.is_in_manual_mode or cs == "SPLIT_SELECT":
            y = self.calib_split_y
            self.image_canvas.create_line(0, y, W, y, fill="red", width=3)
        if cs in ("SKEW_P1", "SKEW_P2"):
            p1 = self.calib_p1
            p2 = self.calib_p2
            self.image_canvas.create_line(p1[0], p1[1], p2[0], p2[1], fill="green", width=2)
            size = 25
            self.image_canvas.create_line(p1[0] - size, p1[1], p1[0] + size, p1[1],
                                          fill="green", width=3)
            self.image_canvas.create_line(p1[0], p1[1] - size, p1[0], p1[1] + size,
                                          fill="green", width=3)
            if cs == "SKEW_P2":
                self.image_canvas.create_line(p2[0] - size, p2[1], p2[0] + size, p2[1],
                                              fill="red", width=3)
                self.image_canvas.create_line(p2[0], p2[1] - size, p2[0], p2[1] + size,
                                              fill="red", width=3)
        elif cs == "START_POINT":
            x = self.calib_start_x
            self.image_canvas.create_line(x, 0, x, H, fill="yellow", width=2)
            if self.is_in_manual_mode:
                y = self.calib_split_y
                self.image_canvas.create_line(0, y, W, y, fill="red", width=3)
                self.image_canvas.create_line(x, y, x, H, fill="yellow", width=2)

    def update_spectrum_loop(self):
        if self._closing:
            return
        try:
            self.render_spectrum()
        except Exception as e:
            print("spectrum err:", e)
        try:
            self.root.after(33, self.update_spectrum_loop)
        except Exception:
            pass

    def render_spectrum(self):
        c = self.spectrum_canvas
        c.delete("all")
        W = max(1, c.winfo_width())
        H = max(1, c.winfo_height())
        self.display_fft_data *= self.spectrum_decay_rate
        self.display_fft_data[self.display_fft_data < 0.001] = 0.0
        np.maximum(self.display_fft_data, self.latest_fft_data, out=self.display_fft_data)
        self.latest_fft_data *= 0.99
        sr = self.current_real_sample_rate
        hz_per_bin = sr / FFT_SIZE
        active_bins = min(len(self.display_fft_data) - 1,
                          int(4000.0 / max(hz_per_bin, 1e-6)))
        if active_bins <= 0:
            return
        bar_w = W / active_bins
        for i in range(active_bins):
            h = float(self.display_fft_data[i]) * H * 0.9
            if h < 1:
                continue
            x = i * bar_w
            c.create_line(x, H, x, H - h, fill="cyan", width=max(1, int(bar_w)))
        fx1 = W * (1500.0 / 4000.0)
        fx2 = W * (2300.0 / 4000.0)
        c.create_line(fx1, 0, fx1, H, fill="red", width=2)
        c.create_line(fx2, 0, fx2, H, fill="red", width=2)
        c.create_text(fx1, 12, text="1500Hz", fill="white", anchor="w")
        c.create_text(fx2, 12, text="2300Hz", fill="white", anchor="w")

    def demodulate_fax(self, buffer, sr):
        buf = np.asarray(buffer, dtype=np.float32)
        n_out = self.image_width
        if len(buf) < 8:
            return np.zeros(n_out, dtype=np.float32)
        samples_per_pixel = len(buf) / n_out
        N = 128
        window = 0.54 - 0.46 * np.cos(2.0 * np.pi * np.arange(N) / (N - 1))
        centers = np.arange(n_out, dtype=np.float64) * samples_per_pixel + samples_per_pixel / 2.0
        starts = (centers - N / 2.0).astype(np.int64)
        idx = starts[:, None] + np.arange(N)[None, :]
        valid = (idx >= 0) & (idx < len(buf))
        idx_clipped = np.clip(idx, 0, len(buf) - 1)
        windows = buf[idx_clipped] * window[None, :] * valid
        fft_result = np.fft.fft(windows, axis=1)
        mags = np.abs(fft_result[:, : N // 2]) ** 2
        start_bin = max(1, int(F_MIN * N / sr) - 2)
        end_bin = min(N // 2 - 1, int(F_MAX * N / sr) + 2)
        if end_bin <= start_bin:
            end_bin = start_bin + 1
        band = mags[:, start_bin:end_bin + 1]
        peak_local = np.argmax(band, axis=1)
        peak_bin = peak_local + start_bin
        freqs = peak_bin.astype(np.float32) * float(sr) / N
        for i in range(n_out):
            pb = int(peak_bin[i])
            if 0 < pb < N // 2 - 1:
                alpha = mags[i, pb - 1]
                beta = mags[i, pb]
                gamma = mags[i, pb + 1]
                denom = alpha - 2 * beta + gamma + 1e-5
                p = 0.5 * (alpha - gamma) / denom
                freqs[i] = (pb + p) * float(sr) / N
        vals = (freqs - F_MIN) / (F_MAX - F_MIN)
        vals = (vals - (1.0 - self.contrast)) / max(self.contrast, 0.001)
        vals = np.clip(vals, 0.0, 1.0)
        return vals.astype(np.float32)

    def compute_fft(self, in_samples):
        x = np.asarray(in_samples, dtype=np.float32) / 32768.0
        N = len(x)
        w = 0.5 * (1.0 - np.cos(2.0 * np.pi * np.arange(N) / (N - 1)))
        xw = x * w
        spec = np.fft.fft(xw)
        mags = np.abs(spec[: N // 2])
        mags_db = 20.0 * np.log10(mags + 1e-4)
        max_log = float(np.max(mags_db))
        mags_norm = (mags_db - (max_log - 60.0)) / 60.0
        np.clip(mags_norm, 0.0, 1.0, out=mags_norm)
        return mags_norm.astype(np.float32)

    def get_segment_for_line(self, line):
        if len(self.sync_segments) > 1:
            for i in range(len(self.sync_segments) - 1, -1, -1):
                seg = self.sync_segments[i]
                if line >= seg.start_line:
                    return seg
        return SyncSegment(0, self.skew_factor, self.global_x_offset)

    def start_decoding(self):
        if self._closing:
            return
        if self.is_decoding:
            self.set_status(self.t("status.decoding_already"))
            return
        try:
            self.pyaudio_instance = pyaudio.PyAudio()
            kwargs = dict(
                format=pyaudio.paInt16,
                channels=1,
                rate=SAMPLE_RATE_LIVE,
                input=True,
                frames_per_buffer=CHUNK_SIZE,
            )
            if self.selected_audio_device is not None:
                kwargs["input_device_index"] = self.selected_audio_device
            self.audio_stream = self.pyaudio_instance.open(**kwargs)
        except Exception as e:
            messagebox.showerror(self.t("app.title"),
                                 f'{self.t("msg.audio_open_failed")}: {e}')
            if self.pyaudio_instance:
                self.pyaudio_instance.terminate()
                self.pyaudio_instance = None
            return
        self.is_decoding = True
        self._set_manual_menu_state("disabled")
        self.set_status(self.t("status.decoding"))
        self.process_thread = threading.Thread(target=self.audio_loop, daemon=True)
        self.process_thread.start()

    def stop_decoding(self):
        if not self.is_decoding:
            return
        self.is_decoding = False
        if self.audio_stream is not None:
            try:
                self.audio_stream.stop_stream()
            except Exception:
                pass
            try:
                self.audio_stream.close()
            except Exception:
                pass
            self.audio_stream = None
        if self.process_thread is not None and self.process_thread.is_alive():
            self.process_thread.join(timeout=2.0)
        self.process_thread = None
        if self.pyaudio_instance is not None:
            try:
                self.pyaudio_instance.terminate()
            except Exception:
                pass
            self.pyaudio_instance = None
        if self.active_calib_window is None and not self._closing:
            self._set_manual_menu_state("normal")
        self.set_status(self.t("status.stopped"))

    def audio_loop(self):
        base_sr = SAMPLE_RATE_LIVE
        self.current_real_sample_rate = base_sr
        resampler = Resampler(base_sr, TARGET_SAMPLE_RATE)
        target_len = int(TARGET_SAMPLE_RATE * self.seconds_per_line)
        line_acc = np.zeros(target_len, dtype=np.float32)
        acc_samples = 0
        fft_ring = np.zeros(FFT_SIZE, dtype=np.float32)
        fft_index = 0
        while self.is_decoding and not self._closing and self.audio_stream is not None:
            try:
                data = self.audio_stream.read(CHUNK_SIZE, exception_on_overflow=False)
            except Exception as e:
                print("read err:", e)
                break
            if not data:
                continue
            chunk = np.frombuffer(data, dtype=np.int16).astype(np.float32)
            n = len(chunk)
            pos = 0
            while pos < n:
                space = FFT_SIZE - fft_index
                take = min(space, n - pos)
                fft_ring[fft_index:fft_index + take] = chunk[pos:pos + take]
                fft_index += take
                pos += take
                if fft_index >= FFT_SIZE:
                    spec = self.compute_fft(fft_ring)
                    np.maximum(self.latest_fft_data, spec, out=self.latest_fft_data)
                    fft_index = 0
            resampler.put(chunk)
            out = resampler.get_output(CHUNK_SIZE * 2)
            if len(out) == 0:
                continue
            i = 0
            while i < len(out):
                space = target_len - acc_samples
                take = min(space, len(out) - i)
                line_acc[acc_samples:acc_samples + take] = out[i:i + take]
                acc_samples += take
                i += take
                if acc_samples >= target_len:
                    self.perform_line_demodulation(line_acc, TARGET_SAMPLE_RATE)
                    target_len = int(TARGET_SAMPLE_RATE * self.seconds_per_line)
                    line_acc = np.zeros(target_len, dtype=np.float32)
                    acc_samples = 0

    def perform_line_demodulation(self, line_data, sr):
        if self._closing:
            return
        row = self.demodulate_fax(line_data, sr)
        with self.data_lock:
            if self._closing:
                return
            idx = self.current_line_index
            if idx >= MAX_LINES:
                return
            self.processed_lines_data.append(row)
            seg = self.get_segment_for_line(idx)
            self.draw_one_line_locked(row, idx, seg.skew_factor, seg.global_x_offset)
            self.current_line_index = idx + 1

    def draw_one_line_locked(self, data, row, skew, offset):
        if row < 0 or row >= MAX_LINES:
            return
        total_shift = row * skew + offset
        eff = ((total_shift % self.image_width) + self.image_width) % self.image_width
        idx = np.arange(self.image_width, dtype=np.float64)
        src_x = idx - eff
        src_x = ((src_x % self.image_width) + self.image_width) % self.image_width
        x1 = src_x.astype(np.int64)
        x2 = (x1 + 1) % self.image_width
        frac = np.clip(src_x - x1, 0.0, 1.0)
        val = data[x1] * (1.0 - frac) + data[x2] * frac
        val = np.clip(val, 0.0, 1.0)
        if self.is_binary_mode:
            g = np.where(val > 0.5, 255, 0).astype(np.uint8)
        else:
            g = (val * 255.0).astype(np.uint8)
        self.image_array[row, :] = g
        self.image_dirty = True

    def draw_one_line(self, data, row, skew, offset):
        with self.data_lock:
            self.draw_one_line_locked(data, row, skew, offset)

    def pick_audio_file(self):
        path = filedialog.askopenfilename(
            title=self.t("dlg.pick_audio.title"),
            filetypes=[
                (self.t("dlg.pick_audio.type"),
                 "*.wav *.flac *.ogg *.aiff *.aif *.au *.raw *.mp3 *.m4a"),
                (self.t("dlg.pick_audio.all"), "*.*"),
            ]
        )
        if path:
            self.selected_file = path
            self.set_status(f'{self.t("status.loading_file")}: {os.path.basename(path)}')

    def start_file_decoding(self):
        if self._closing:
            return
        if not self.selected_file:
            messagebox.showinfo(self.t("app.title"), self.t("msg.please_select_file"))
            return
        if self.is_decoding:
            messagebox.showinfo(self.t("app.title"), self.t("msg.stop_first"))
            return
        if self.file_decode_thread is not None and self.file_decode_thread.is_alive():
            messagebox.showinfo(self.t("app.title"), self.t("msg.file_decode_running"))
            return
        if self.active_calib_window is not None:
            messagebox.showwarning(self.t("app.title"), self.t("msg.exit_calib_first"))
            return

        self.clear_all()
        path = self.selected_file
        self.active_calib_window = "file"
        self.lock_calib_menus()
        self.set_status(self.t("status.file_decoding"))

        def worker():
            try:
                self.decode_audio_file(path)
                if self._closing:
                    return
                self.root.after(0, lambda: messagebox.showinfo(
                    self.t("app.title"), self.t("status.file_done")))
                self.root.after(0, lambda: self.set_status(self.t("status.file_done")))
            except Exception as e:
                import traceback
                traceback.print_exc()
                if self._closing:
                    return
                err = str(e)
                self.root.after(0, lambda: messagebox.showerror(
                    self.t("msg.decode_failed"), err))
                self.root.after(0, lambda: self.set_status(
                    f'{self.t("status.file_failed")}: {err}'))
            finally:
                self.active_calib_window = None
                if self._closing:
                    return

                def _cleanup():
                    if self._closing:
                        return
                    self.unlock_calib_menus()
                    self._set_manual_menu_state("disabled" if self.is_decoding else "normal")

                try:
                    self.root.after(0, _cleanup)
                except Exception:
                    pass

        self.file_decode_thread = threading.Thread(target=worker, daemon=True)
        self.file_decode_thread.start()

    def _load_audio_samples(self, path):
        data, sr = sf.read(path, dtype="int16", always_2d=True)
        samples = data[:, 0]
        return samples.astype(np.int16), int(sr)

    def decode_audio_file(self, path):
        samples, sr = self._load_audio_samples(path)
        self.current_real_sample_rate = sr
        resampler = Resampler(sr, TARGET_SAMPLE_RATE)
        target_len = int(TARGET_SAMPLE_RATE * self.seconds_per_line)
        line_acc = np.zeros(target_len, dtype=np.float32)
        acc_samples = 0
        in_chunk = 48000
        n_total = len(samples)
        for start in range(0, n_total, in_chunk):
            if self._closing:
                return
            seg = samples[start:start + in_chunk].astype(np.float32)
            resampler.put(seg)
            while True:
                if self._closing:
                    return
                out = resampler.get_output(in_chunk * 2)
                if len(out) == 0:
                    break
                i = 0
                while i < len(out):
                    space = target_len - acc_samples
                    take = min(space, len(out) - i)
                    line_acc[acc_samples:acc_samples + take] = out[i:i + take]
                    acc_samples += take
                    i += take
                    if acc_samples >= target_len:
                        self.perform_line_demodulation(line_acc, TARGET_SAMPLE_RATE)
                        target_len = int(TARGET_SAMPLE_RATE * self.seconds_per_line)
                        line_acc = np.zeros(target_len, dtype=np.float32)
                        acc_samples = 0
        while not self._closing:
            out = resampler.get_output(in_chunk * 2)
            if len(out) == 0:
                break
            i = 0
            while i < len(out):
                if self._closing:
                    return
                space = target_len - acc_samples
                take = min(space, len(out) - i)
                line_acc[acc_samples:acc_samples + take] = out[i:i + take]
                acc_samples += take
                i += take
                if acc_samples >= target_len:
                    self.perform_line_demodulation(line_acc, TARGET_SAMPLE_RATE)
                    target_len = int(TARGET_SAMPLE_RATE * self.seconds_per_line)
                    line_acc = np.zeros(target_len, dtype=np.float32)
                    acc_samples = 0

    def redraw_all_history(self):
        def worker():
            with self.data_lock:
                copy = list(self.processed_lines_data)
            for i, row in enumerate(copy):
                if self._closing:
                    return
                seg = self.get_segment_for_line(i)
                self.draw_one_line(row, i, seg.skew_factor, seg.global_x_offset)
            with self.data_lock:
                self.current_line_index = len(copy)
            self.mark_image_dirty()
        threading.Thread(target=worker, daemon=True).start()

    def redraw_preview_with_temp(self):
        def worker():
            with self.data_lock:
                copy = list(self.processed_lines_data)
            for i, row in enumerate(copy):
                if self._closing:
                    return
                if i < self.temp_split_line:
                    seg = self.get_segment_for_line(i)
                    self.draw_one_line(row, i, seg.skew_factor, seg.global_x_offset)
                else:
                    self.draw_one_line(row, i, self.temp_skew, self.temp_offset)
            with self.data_lock:
                self.current_line_index = len(copy)
            self.mark_image_dirty()
        threading.Thread(target=worker, daemon=True).start()

    def save_image(self):
        if self.current_line_index == 0:
            messagebox.showinfo(self.t("app.title"), self.t("msg.no_image"))
            return
        with self.data_lock:
            img_data = self.image_array[:self.current_line_index, :].copy()
        ts = datetime.now().strftime("%Y%m%d_%H%M%S")
        default_name = f"FAX_{ts}.png"
        path = filedialog.asksaveasfilename(
            title=self.t("dlg.save_image.title"),
            defaultextension=".png",
            initialfile=default_name,
            filetypes=[
                (self.t("dlg.save_image.png"), "*.png"),
                (self.t("dlg.save_image.jpg"), "*.jpg *.jpeg"),
                (self.t("dlg.save_image.tiff"), "*.tiff *.tif"),
                (self.t("dlg.save_image.raw"), "*.raw"),
            ]
        )
        if not path:
            return
        ext = os.path.splitext(path)[1].lower()
        try:
            fd, temp_path = tempfile.mkstemp(suffix=ext or ".png",
                                             prefix="fax_img_",
                                             dir=tempfile.gettempdir())
            os.close(fd)
            if ext == ".raw":
                data16 = (img_data.astype(np.uint16) * 257)
                data16.tofile(temp_path)
            else:
                pil_img = Image.fromarray(img_data, mode="L")
                if ext in (".jpg", ".jpeg"):
                    pil_img.save(temp_path, "JPEG", quality=95)
                elif ext in (".tif", ".tiff"):
                    pil_img.save(temp_path, "TIFF")
                else:
                    pil_img.save(temp_path, "PNG")
            shutil.copyfile(temp_path, path)
            try:
                os.remove(temp_path)
            except Exception:
                pass
            self.set_status(f'{self.t("status.image_saved")}: {path}')
            messagebox.showinfo(self.t("msg.save_ok_title"),
                                f'{self.t("status.image_saved")}:\n{path}')
        except Exception as e:
            messagebox.showerror(self.t("app.title"),
                                 f'{self.t("msg.save_failed")}: {e}')

    def open_record_window(self):
        if self.record_window is not None and self.record_window.winfo_exists():
            self.record_window.deiconify()
            self.record_window.lift()
            self.update_record_window()
            return
        self.record_window = tk.Toplevel(self.root)
        self.record_window.title(self.t("dlg.record.title"))
        self.record_window.geometry("340x220")
        self.record_window.transient(self.root)
        self.record_window.protocol("WM_DELETE_WINDOW", self.hide_record_window)
        self.record_window.configure(bg="gray25")
        self.record_window.resizable(False, False)
        tk.Label(self.record_window, text=self.t("dlg.record.header"),
                 bg="gray25", fg="white", font=("Arial", 14, "bold")).pack(pady=(16, 6))
        self.record_status_label = tk.Label(self.record_window, text="", bg="gray25",
                                            fg="#FFEB3B", font=("Arial", 11))
        self.record_status_label.pack(pady=6)
        self.record_action_btn = tk.Button(
            self.record_window, text=self.t("dlg.record.start"),
            command=self.toggle_recording,
            bg="#2E7D32", fg="white", activebackground="#1B5E20",
            activeforeground="white", font=("Arial", 12, "bold"),
            height=2, width=18, relief=tk.RAISED, bd=2
        )
        self.record_action_btn.pack(pady=14)
        tk.Label(self.record_window, text=self.t("dlg.record.hint"),
                 bg="gray25", fg="lightgray", font=("Arial", 8)).pack(pady=(4, 10))
        self.update_record_window()

    def hide_record_window(self):
        if self.record_window is not None and self.record_window.winfo_exists():
            self.record_window.withdraw()

    def update_record_window(self):
        if self.record_window is None or not self.record_window.winfo_exists():
            return
        if self.is_recording_audio:
            self.record_status_label.config(text=self.t("dlg.record.recording"), fg="#FF5252")
            self.record_action_btn.config(text=self.t("dlg.record.stop"), bg="#C62828",
                                          activebackground="#8E0000")
        else:
            self.record_status_label.config(text=self.t("dlg.record.not_recording"), fg="#FFEB3B")
            self.record_action_btn.config(text=self.t("dlg.record.start"), bg="#2E7D32",
                                          activebackground="#1B5E20")

    def toggle_recording(self):
        if self.is_recording_audio:
            self.hide_record_window()
            self.stop_recording_save()
        else:
            self.start_recording_audio()
            if self.is_recording_audio:
                self.hide_record_window()
            self.update_record_window()

    def start_recording_audio(self):
        try:
            self.record_pyaudio = pyaudio.PyAudio()
            kwargs = dict(
                format=pyaudio.paInt16,
                channels=1,
                rate=SAMPLE_RATE_LIVE,
                input=True,
                frames_per_buffer=CHUNK_SIZE,
            )
            if self.selected_audio_device is not None:
                kwargs["input_device_index"] = self.selected_audio_device
            self.record_stream = self.record_pyaudio.open(**kwargs)
        except Exception as e:
            messagebox.showerror(self.t("app.title"),
                                 f'{self.t("msg.recording_open_failed")}: {e}')
            if self.record_pyaudio:
                try:
                    self.record_pyaudio.terminate()
                except Exception:
                    pass
                self.record_pyaudio = None
            self.record_stream = None
            return
        try:
            fd, self.record_temp_path = tempfile.mkstemp(
                suffix=".wav", prefix="fax_record_", dir=tempfile.gettempdir())
            os.close(fd)
        except Exception as e:
            messagebox.showerror(self.t("app.title"),
                                 f'{self.t("msg.temp_file_failed")}: {e}')
            try:
                self.record_stream.stop_stream()
                self.record_stream.close()
            except Exception:
                pass
            self.record_stream = None
            try:
                self.record_pyaudio.terminate()
            except Exception:
                pass
            self.record_pyaudio = None
            return
        self.is_recording_audio = True
        self.set_status(self.t("status.recording"))
        self.record_thread = threading.Thread(target=self.record_loop, daemon=True)
        self.record_thread.start()

    def record_loop(self):
        wf = None
        try:
            wf = wave.open(self.record_temp_path, "wb")
            wf.setnchannels(1)
            wf.setsampwidth(2)
            wf.setframerate(SAMPLE_RATE_LIVE)
            while self.is_recording_audio and not self._closing and self.record_stream is not None:
                try:
                    data = self.record_stream.read(CHUNK_SIZE, exception_on_overflow=False)
                except Exception as e:
                    print("record read err:", e)
                    break
                if data:
                    wf.writeframes(data)
        finally:
            if wf is not None:
                try:
                    wf.close()
                except Exception:
                    pass

    def stop_recording_save(self):
        self.is_recording_audio = False
        if self.record_thread is not None and self.record_thread.is_alive():
            self.record_thread.join(timeout=2.0)
        self.record_thread = None
        if self.record_stream is not None:
            try:
                self.record_stream.stop_stream()
                self.record_stream.close()
            except Exception:
                pass
            self.record_stream = None
        if self.record_pyaudio is not None:
            try:
                self.record_pyaudio.terminate()
            except Exception:
                pass
            self.record_pyaudio = None
        self.set_status(self.t("status.recording_done"))
        if self._closing:
            try:
                if self.record_temp_path and os.path.exists(self.record_temp_path):
                    os.remove(self.record_temp_path)
            except Exception:
                pass
            self.record_temp_path = None
            return
        ts = datetime.now().strftime("%Y%m%d_%H%M%S")
        default_name = f"FAX_record_{ts}.wav"
        path = filedialog.asksaveasfilename(
            title=self.t("dlg.save_audio.title"),
            defaultextension=".wav",
            initialfile=default_name,
            filetypes=[(self.t("dlg.save_audio.type"), "*.wav")]
        )
        if path:
            try:
                shutil.copyfile(self.record_temp_path, path)
                messagebox.showinfo(self.t("msg.save_ok_title"),
                                    f'{self.t("status.recording_saved")}:\n{path}')
                self.set_status(f'{self.t("status.recording_saved")}: {path}')
            except Exception as e:
                messagebox.showerror(self.t("app.title"),
                                     f'{self.t("msg.save_failed")}: {e}')
        else:
            self.set_status(self.t("status.save_cancelled"))
        try:
            if self.record_temp_path and os.path.exists(self.record_temp_path):
                os.remove(self.record_temp_path)
        except Exception:
            pass
        self.record_temp_path = None
        self.update_record_window()

    def reinit_bitmap(self):
        with self.data_lock:
            self.image_array = np.zeros((MAX_LINES, self.image_width), dtype=np.uint8)
            self.current_line_index = 0
            self.processed_lines_data = []
            self.sync_segments = [SyncSegment(0, 0.0, 0.0)]
            self.skew_factor = 0.0
            self.global_x_offset = 0.0
        self.mark_image_dirty()

    def clear_all(self):
        if self.active_calib_window is not None and self.active_calib_window != "file":
            self.calib_state = "NONE"
            self.is_in_manual_mode = False
            self.active_calib_window = None
            self.hide_calib_bar()
            self.unlock_calib_menus()
            self._set_manual_menu_state("disabled" if self.is_decoding else "normal")
        with self.data_lock:
            self.current_line_index = 0
            self.processed_lines_data = []
            self.sync_segments = [SyncSegment(0, 0.0, 0.0)]
            self.skew_factor = 0.0
            self.global_x_offset = 0.0
            self.temp_skew = 0.0
            self.temp_offset = 0.0
            self.temp_split_line = 0
            self.image_array[:] = 0
        self.view_scale = 1.0
        self.view_offset_x = 0.0
        self.view_offset_y = 0.0
        self.mark_image_dirty()
        self.set_status(self.t("status.cleared"))

    def on_close(self):
        if self._closing:
            return
        self._closing = True
        self.is_recording_audio = False
        self.is_decoding = False
        for s in (self.record_stream, self.audio_stream):
            if s is not None:
                try:
                    s.stop_stream()
                except Exception:
                    pass
                try:
                    s.close()
                except Exception:
                    pass
        self.record_stream = None
        self.audio_stream = None
        for t in (self.record_thread, self.process_thread, self.file_decode_thread):
            if t is not None and t.is_alive():
                t.join(timeout=2.0)
        for p in (self.record_pyaudio, self.pyaudio_instance):
            if p is not None:
                try:
                    p.terminate()
                except Exception:
                    pass
        self.record_pyaudio = None
        self.pyaudio_instance = None
        if self.record_temp_path and os.path.exists(self.record_temp_path):
            try:
                os.remove(self.record_temp_path)
            except Exception:
                pass
        try:
            self.root.destroy()
        except Exception:
            pass

    def run(self):
        self.root.protocol("WM_DELETE_WINDOW", self.on_close)
        self.root.mainloop()


if __name__ == "__main__":
    app = FaxDecoderApp()
    app.run()