#pragma once
#include <string>
#include <cstdint>

namespace ImageIO {

std::string saveByExtension(const std::string& path,
                            const uint8_t* pixels, int width, int height);

std::string saveImage(const std::string& path,
                      const uint8_t* pixels, int width, int height,
                      int jpeg_quality = 95);

std::string saveRaw16(const std::string& path,
                      const uint8_t* pixels, int width, int height);

std::string lowerExt(const std::string& path);

}