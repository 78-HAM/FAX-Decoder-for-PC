#pragma once
#include <string>

struct UpdateInfo {
    bool checked      = false;
    bool success      = false;
    bool has_update   = false;
    std::string current_version;
    std::string latest_version;
    std::string latest_url;
    std::string error;
};

class Updater {
public:
    static constexpr const char* kReleasesApi  = "https://api.github.com/repos/78-HAM/FAX-Decoder-for-PC/releases";
    static constexpr const char* kTagsApi      = "https://api.github.com/repos/78-HAM/FAX-Decoder-for-PC/tags";
    static constexpr const char* kReleasesPage = "https://github.com/78-HAM/FAX-Decoder-for-PC/releases";

    static UpdateInfo check(const std::string& current_version);
    static int compareVersions(const std::string& a, const std::string& b);
};