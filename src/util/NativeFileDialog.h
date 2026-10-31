#pragma once
#include <string>
#include <vector>

namespace NativeFileDialog {
    std::string openFile(const std::string& title, const std::string& filter);
    std::string saveFile(const std::string& title, const std::string& defaultName, const std::string& filter);
}
