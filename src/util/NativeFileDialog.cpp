#include "util/NativeFileDialog.h"
#include <cstdio>
#include <cstdlib>

#ifdef _WIN32
#include <windows.h>
#include <commdlg.h>

std::string NativeFileDialog::openFile(const std::string& title, const std::string& filter) {
    OPENFILENAMEA ofn;
    char szFile[260] = {0};
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    std::string f = filter + '\0'; // double null terminated
    ofn.lpstrFilter = f.c_str();
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = NULL;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
    ofn.lpstrTitle = title.c_str();
    if (GetOpenFileNameA(&ofn) == TRUE) return std::string(szFile);
    return "";
}

std::string NativeFileDialog::saveFile(const std::string& title, const std::string& defaultName, const std::string& filter) {
    OPENFILENAMEA ofn;
    char szFile[260] = {0};
    strncpy(szFile, defaultName.c_str(), sizeof(szFile) - 1);
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    std::string f = filter + '\0';
    ofn.lpstrFilter = f.c_str();
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = NULL;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;
    ofn.lpstrTitle = title.c_str();
    if (GetSaveFileNameA(&ofn) == TRUE) return std::string(szFile);
    return "";
}

#else // Linux / macOS fallback
std::string NativeFileDialog::openFile(const std::string& title, const std::string& filter) {
    std::string cmd = "zenity --file-selection --title=\"" + title + "\" 2>/dev/null";
    FILE* f = popen(cmd.c_str(), "r");
    if (!f) return "";
    char buf[1024];
    std::string res = "";
    if (fgets(buf, sizeof(buf), f)) {
        res = buf;
        if (!res.empty() && res.back() == '\n') res.pop_back();
    }
    pclose(f);
    return res;
}

std::string NativeFileDialog::saveFile(const std::string& title, const std::string& defaultName, const std::string& filter) {
    std::string cmd = "zenity --file-selection --save --confirm-overwrite --title=\"" + title + "\" --filename=\"" + defaultName + "\" 2>/dev/null";
    FILE* f = popen(cmd.c_str(), "r");
    if (!f) return "";
    char buf[1024];
    std::string res = "";
    if (fgets(buf, sizeof(buf), f)) {
        res = buf;
        if (!res.empty() && res.back() == '\n') res.pop_back();
    }
    pclose(f);
    return res;
}
#endif
