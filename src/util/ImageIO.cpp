#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include "util/ImageIO.h"
#include <cstdio>
#include <cstdint>
#include <vector>
#include <algorithm>
#include <cctype>

namespace ImageIO {

std::string lowerExt(const std::string& path) {
    auto dot = path.find_last_of('.');
    if (dot == std::string::npos) return "";
    std::string ext = path.substr(dot);
    std::transform(ext.begin(), ext.end(), ext.begin(),
                   [](unsigned char c) { return (char)std::tolower(c); });
    return ext;
}

std::string saveImage(const std::string& path,
                      const uint8_t* pixels, int width, int height,
                      int jpeg_quality) {
    if (!pixels || width <= 0 || height <= 0) return "empty image";

    std::string ext = lowerExt(path);
    int ok = 0;

    if (ext == ".png" || ext.empty()) {
        ok = stbi_write_png(path.c_str(), width, height, 1, pixels, width);
    } else if (ext == ".jpg" || ext == ".jpeg") {
        ok = stbi_write_jpg(path.c_str(), width, height, 1, pixels,
                            std::clamp(jpeg_quality, 1, 100));
    } else if (ext == ".bmp") {
        ok = stbi_write_bmp(path.c_str(), width, height, 1, pixels);
    } else if (ext == ".tga") {
        ok = stbi_write_tga(path.c_str(), width, height, 1, pixels);
    } else {
        return "unsupported format: " + ext;
    }

    return ok ? std::string() : std::string("stbi_write failed");
}

std::string saveRaw16(const std::string& path,
                      const uint8_t* pixels, int width, int height) {
    if (!pixels || width <= 0 || height <= 0) return "empty image";

    FILE* fp = std::fopen(path.c_str(), "wb");
    if (!fp) return "cannot open file for writing";

    size_t n = (size_t)width * (size_t)height;
    std::vector<uint16_t> buf(n);
    for (size_t i = 0; i < n; ++i) buf[i] = (uint16_t)pixels[i] * 257u;

    size_t written = std::fwrite(buf.data(), sizeof(uint16_t), n, fp);
    std::fclose(fp);

    return (written == n) ? std::string() : std::string("short write");
}

std::string saveByExtension(const std::string& path,
                            const uint8_t* pixels, int width, int height) {
    std::string ext = lowerExt(path);
    if (ext == ".raw") return saveRaw16(path, pixels, width, height);
    return saveImage(path, pixels, width, height);
}

}