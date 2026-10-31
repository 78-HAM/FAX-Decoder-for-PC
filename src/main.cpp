#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812F
#endif
#ifndef GL_R8
#define GL_R8 0x8229
#endif
#ifndef GL_RED
#define GL_RED 0x1903
#endif

#include <GLFW/glfw3.h>
#include "core/init.h"
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <cmath>
#include <string>
#include <tuple>

static void glfw_error_cb(int err, const char* desc) {
    std::fprintf(stderr, "GLFW Error %d: %s\n", err, desc);
}

static GLuint createTestTexture(int w, int h) {
    std::vector<unsigned char> data(w * h * 4);
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            int i = (y * w + x) * 4;
            data[i + 0] = (unsigned char)((x * 255) / w);
            data[i + 1] = (unsigned char)((y * 255) / h);
            data[i + 2] = 80;
            data[i + 3] = 255;
        }
    }
    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data.data());
    return tex;
}

struct GlTexture { GLuint id = 0; int w = 0, h = 0; };
static GlTexture g_img_tex;

static ImTextureID uploadGrayTexture(const uint8_t* pixels, int w, int h) {
    if (!pixels || w <= 0 || h <= 0) return (ImTextureID)0;
    std::vector<uint8_t> rgba(w * h * 4);
    for (int i = 0; i < w * h; ++i) {
        rgba[i * 4 + 0] = pixels[i];
        rgba[i * 4 + 1] = pixels[i];
        rgba[i * 4 + 2] = pixels[i];
        rgba[i * 4 + 3] = 255;
    }
    if (g_img_tex.id == 0 || g_img_tex.w != w || g_img_tex.h < h) {
        if (g_img_tex.id) glDeleteTextures(1, &g_img_tex.id);
        glGenTextures(1, &g_img_tex.id);
        glBindTexture(GL_TEXTURE_2D, g_img_tex.id);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        
        int alloc_h = std::max(5000, h);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, alloc_h, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        g_img_tex.w = w;
        g_img_tex.h = alloc_h;
    }
    
    glBindTexture(GL_TEXTURE_2D, g_img_tex.id);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, rgba.data());
    return (ImTextureID)(intptr_t)g_img_tex.id;
}

int main(int, char**) {
    glfwSetErrorCallback(glfw_error_cb);
    if (!glfwInit()) {
        std::system("pause");
        return 1;
    }

    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

    GLFWwindow* window = glfwCreateWindow(1400, 820, "FAX Decoder 2.0", nullptr, nullptr);
    if (!window) {
        std::system("pause");
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGuiIO& io = ImGui::GetIO();

    // 字体加载辅助函数
    auto tryLoadFont = [](const char* path, const ImWchar* ranges) -> ImFont* {
        FILE* f = fopen(path, "rb");
        if (f) {
            fclose(f);
            ImFont* font = ImGui::GetIO().Fonts->AddFontFromFileTTF(path, 20.0f, nullptr, ranges);
            if (font) return font;
        }
        return nullptr;
    };

    auto reloadFonts = [&](const std::string& custom_font_path) -> std::tuple<ImFont*, ImFont*, ImFont*> {
        io.Fonts->Clear();
        
        ImFont* f_en = tryLoadFont("resources/fonts/Arial.ttf", io.Fonts->GetGlyphRangesCyrillic());
        if (!f_en) f_en = io.Fonts->AddFontDefault();
        
        // 使用 Common (3500常用字) 既省内存又稳
        ImFont* f_zh = tryLoadFont(custom_font_path.c_str(), io.Fonts->GetGlyphRangesChineseSimplifiedCommon());
        if (!f_zh) f_zh = tryLoadFont("C:/Windows/Fonts/msyh.ttc", io.Fonts->GetGlyphRangesChineseSimplifiedCommon());
        if (!f_zh) f_zh = f_en;
        
        ImFont* f_jp = tryLoadFont("C:/Windows/Fonts/msgothic.ttc", io.Fonts->GetGlyphRangesJapanese());
        if (!f_jp) f_jp = f_zh;
        
        return {f_en, f_zh, f_jp};
    };

    std::string current_custom_font = "resources/fonts/Roboto-Medium.ttf";
    if (FILE* f = fopen(current_custom_font.c_str(), "rb")) { fclose(f); } 
    else { current_custom_font = "C:/Windows/Fonts/msyh.ttc"; }

    auto [font_en, font_zh, font_jp] = reloadFonts(current_custom_font);

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    GLuint test_tex = createTestTexture(600, 400);

    App app;
    app.setTextureUploader(uploadGrayTexture);
    app.init();
    app.setChineseAvailable(true);

    {
        std::vector<float> demo_line(app.settings().image_width);
        for (int row = 0; row < 200; ++row) {
            for (int x = 0; x < (int)demo_line.size(); ++x) {
                float fx = (float)x / (float)demo_line.size();
                demo_line[x] = (fx * 0.5f) + (std::sin((float)row * 0.05f + fx * 10.0f) * 0.25f + 0.25f);
            }
            app.pushRawLine(demo_line.data(), 48000);
        }
    }

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        // 安全地在 NewFrame 之前处理字体重载
        std::string pending = app.takePendingFontPath();
        if (!pending.empty()) {
            current_custom_font = pending;
            std::tie(font_en, font_zh, font_jp) = reloadFonts(current_custom_font);
            // 删除旧纹理，创建新纹理（必须在 NewFrame 之前）
            ImGui_ImplOpenGL3_DestroyFontsTexture();
            ImGui_ImplOpenGL3_CreateFontsTexture();
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        std::string lang = app.settings().language;
        if (lang == "zh-cn" || lang == "zh_CN" || lang == "zh-tw" || lang == "zh_TW") {
            ImGui::PushFont(font_zh);
        } else if (lang == "jp" || lang == "ja") {
            ImGui::PushFont(font_jp);
        } else {
            ImGui::PushFont(font_en);
        }
        
        app.update();
        ImGui::PopFont();

        if (app.wantsClose()) glfwSetWindowShouldClose(window, GLFW_TRUE);

        ImGui::Render();
        int w, h;
        glfwGetFramebufferSize(window, &w, &h);
        glViewport(0, 0, w, h);
        glClearColor(0.10f, 0.10f, 0.10f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    if (g_img_tex.id) glDeleteTextures(1, &g_img_tex.id);
    glDeleteTextures(1, &test_tex);
    app.shutdown();

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
