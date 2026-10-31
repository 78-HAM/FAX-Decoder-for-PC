#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>

#include "core/init.h"

#include <cstdio>
#include <cstdlib>
#include <vector>

static void glfw_error_cb(int err, const char* desc) {
    std::fprintf(stderr, "GLFW Error %d: %s\n", err, desc);
}

struct GlTexture {
    GLuint id = 0;
    int w = 0, h = 0;
};

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
        g_img_tex.w = w;
        g_img_tex.h = h;
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

    std::printf("[2] Creating window...\n"); std::fflush(stdout);
    GLFWwindow* window = glfwCreateWindow(1400, 820, "FAX Decoder", nullptr, nullptr);
    if (!window) {
        std::printf("[ERROR] Window creation failed. Your GPU may not support OpenGL 3.0.\n");
        std::system("pause");
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    std::printf("[3] Init ImGui...\n"); std::fflush(stdout);
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    std::printf("[4] Init App core...\n"); std::fflush(stdout);
    GLuint test_tex = createTestTexture(600, 400);
    // ...此处省略 createTestTexture 定义，和之前一样...
    // 如果原来的 main.cpp 有 createTestTexture，请保留它。

    App app;
    app.setTextureUploader(uploadGrayTexture);
    app.init();

    std::printf("[5] App initialized. Entering loop...\n"); std::fflush(stdout);

    // (保留你原来的 demo image 生成代码)

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        app.update();

        if (app.wantsClose()) {
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        }

        ImGui::Render();
        int w, h;
        glfwGetFramebufferSize(window, &w, &h);
        glViewport(0, 0, w, h);
        glClearColor(0.10f, 0.10f, 0.10f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    std::printf("[6] Exiting loop...\n"); std::fflush(stdout);

    if (g_img_tex.id) glDeleteTextures(1, &g_img_tex.id);
    app.shutdown();

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();

    std::printf("[7] Done. Press any key to exit...\n");
    std::system("pause");
    return 0;
}