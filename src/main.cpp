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
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data.data());
    return tex;
}

struct GlTexture { GLuint id = 0; int w = 0, h = 0; };
static GlTexture g_img_tex;

static ImTextureID uploadGrayTexture(const uint8_t* pixels, int w, int h) {
    if (!pixels || w <= 0 || h <= 0) return (ImTextureID)0;
    if (g_img_tex.id == 0 || g_img_tex.w != w || g_img_tex.h != h) {
        if (g_img_tex.id) glDeleteTextures(1, &g_img_tex.id);
        glGenTextures(1, &g_img_tex.id);
        glBindTexture(GL_TEXTURE_2D, g_img_tex.id);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, w, h, 0, GL_RED, GL_UNSIGNED_BYTE, pixels);
        g_img_tex.w = w; g_img_tex.h = h;
    } else {
        glBindTexture(GL_TEXTURE_2D, g_img_tex.id);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h, GL_RED, GL_UNSIGNED_BYTE, pixels);
    }
    return (ImTextureID)(intptr_t)g_img_tex.id;
}

int main(int, char**) {
    std::printf("[1] App starting...\n"); std::fflush(stdout);

    glfwSetErrorCallback(glfw_error_cb);
    if (!glfwInit()) {
        std::printf("[ERROR] GLFW Init failed.\n");
        std::system("pause");
        return 1;
    }

    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

    GLFWwindow* window = glfwCreateWindow(1400, 820, "FAX Decoder", nullptr, nullptr);
    if (!window) {
        std::printf("[ERROR] Window creation failed.\n");
        std::system("pause");
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();

    // --- 加载中文字体 ---
    ImGuiIO& io = ImGui::GetIO();
    bool font_loaded = false;
    
    // 注意你的文件夹名字是 fount
    ImFont* font = io.Fonts->AddFontFromFileTTF("fount/Roboto-Medium.ttf", 20.0f, nullptr, io.Fonts->GetGlyphRangesChineseFull());
    if (!font) {
        std::printf("[WARN] Failed to load fount/Roboto-Medium.ttf, using default font.\n");
        io.Fonts->AddFontDefault();
    } else {
        std::printf("[INFO] Chinese font loaded.\n");
        font_loaded = true;
    }

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    GLuint test_tex = createTestTexture(600, 400);

    App app;
    app.setTextureUploader(uploadGrayTexture);
    app.init();
    
    // 告诉 UI 层：中文字体已就绪，解锁语言菜单
    app.setChineseAvailable(font_loaded);

    // 默认演示图像
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
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        app.update();

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
