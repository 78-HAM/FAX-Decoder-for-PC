#include "util/Updater.h"
#include <curl/curl.h>
#include "json.hpp"
#include <regex>
#include <vector>
#include <algorithm>
#include <cctype>
#include <mutex>

namespace {

size_t writeCb(char* ptr, size_t size, size_t nmemb, void* userdata) {
    auto* s = static_cast<std::string*>(userdata);
    s->append(ptr, size * nmemb);
    return size * nmemb;
}

std::once_flag g_curl_once;
void ensureCurlInit() {
    std::call_once(g_curl_once, []() { curl_global_init(CURL_GLOBAL_DEFAULT); });
}

bool httpGet(const std::string& url, std::string& out, std::string& err, long timeout_sec = 8) {
    ensureCurlInit();
    CURL* curl = curl_easy_init();
    if (!curl) { err = "curl_easy_init failed"; return false; }
    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "User-Agent: Mozilla/5.0 FAX-Decoder-Updater");
    headers = curl_slist_append(headers, "Accept: application/vnd.github+json");
    headers = curl_slist_append(headers, "Cache-Control: no-cache");
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &out);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeout_sec);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 8L);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    CURLcode res = curl_easy_perform(curl);
    long code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    if (res != CURLE_OK) { err = curl_easy_strerror(res); return false; }
    if (code >= 400)     { err = "HTTP " + std::to_string(code); return false; }
    return true;
}

std::vector<int> parseVersionParts(const std::string& v) {
    std::vector<int> parts;
    std::regex re("(\\d+)");
    for (auto it = std::sregex_iterator(v.begin(), v.end(), re),
              end = std::sregex_iterator(); it != end; ++it) {
        try { parts.push_back(std::stoi(it->str())); } catch (...) { parts.push_back(0); }
    }
    if (parts.empty()) parts.push_back(0);
    return parts;
}

}

int Updater::compareVersions(const std::string& a, const std::string& b) {
    auto pa = parseVersionParts(a);
    auto pb = parseVersionParts(b);
    size_t n = std::max(pa.size(), pb.size());
    pa.resize(n, 0);
    pb.resize(n, 0);
    for (size_t i = 0; i < n; ++i) {
        if (pa[i] < pb[i]) return -1;
        if (pa[i] > pb[i]) return  1;
    }
    return 0;
}

UpdateInfo Updater::check(const std::string& current_version) {
    UpdateInfo info;
    info.checked = true;
    info.current_version = current_version;

    std::string latest_ver;
    std::string latest_url = kReleasesPage;
    std::string err;
    bool got_any = false;

    std::string body;
    if (httpGet(kReleasesApi, body, err)) {
        got_any = true;
        try {
            auto j = nlohmann::json::parse(body);
            if (j.is_array()) {
                for (auto& rel : j) {
                    std::string tag = rel.value("tag_name", std::string());
                    if (tag.empty()) tag = rel.value("name", std::string());
                    if (tag.empty()) continue;
                    if (latest_ver.empty() || compareVersions(tag, latest_ver) > 0) {
                        latest_ver = tag;
                        latest_url = rel.value("html_url", std::string(kReleasesPage));
                    }
                }
            }
        } catch (...) {}
    }

    if (latest_ver.empty()) {
        body.clear();
        if (httpGet(kTagsApi, body, err)) {
            got_any = true;
            try {
                auto j = nlohmann::json::parse(body);
                if (j.is_array()) {
                    for (auto& t : j) {
                        std::string name = t.value("name", std::string());
                        if (name.empty()) continue;
                        if (latest_ver.empty() || compareVersions(name, latest_ver) > 0) {
                            latest_ver = name;
                            latest_url = std::string(kReleasesPage) + "/tag/" + name;
                        }
                    }
                }
            } catch (...) {}
        }
    }

    if (!got_any || latest_ver.empty()) {
        info.success = false;
        info.error   = err.empty() ? "no version found" : err;
        return info;
    }

    info.success        = true;
    info.latest_version = latest_ver;
    info.latest_url     = latest_url;
    info.has_update     = compareVersions(latest_ver, current_version) > 0;
    return info;
}