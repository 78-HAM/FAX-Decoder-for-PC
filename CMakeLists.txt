cmake_minimum_required(VERSION 3.16)
project(fax_decoder LANGUAGES CXX C)
if(WIN32)
    enable_language(RC)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GLFW glfw3)
if(NOT GLFW_FOUND)
    pkg_check_modules(GLFW glfw)
endif()
if(NOT GLFW_FOUND)
    message(FATAL_ERROR "GLFW not found (tried glfw3 and glfw)")
endif()

pkg_check_modules(PORTAUDIO portaudio-2.0)
find_package(CURL REQUIRED)
find_package(Threads REQUIRED)
find_package(OpenGL QUIET)

# ---------------- ImGui ----------------
set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/third_party/imgui)
add_library(imgui STATIC
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)
target_include_directories(imgui PUBLIC
    ${IMGUI_DIR} ${IMGUI_DIR}/backends ${GLFW_INCLUDE_DIRS})
target_link_libraries(imgui PUBLIC ${GLFW_LIBRARIES} Threads::Threads)

# ---------------- ImGuiFileDialog（可选） ----------------
set(IFD_DIR ${CMAKE_SOURCE_DIR}/third_party/ImGuiFileDialog)
if(EXISTS ${IFD_DIR}/ImGuiFileDialog.cpp AND EXISTS ${IFD_DIR}/ImGuiFileDialog.h)
    add_library(imgui_filedialog STATIC ${IFD_DIR}/ImGuiFileDialog.cpp)
    # ★ 这里补上了 third_party 和 third_party/dirent，解决 Windows 找不到 dirent.h 的问题
    target_include_directories(imgui_filedialog PUBLIC 
        ${IFD_DIR} 
        ${IMGUI_DIR} 
        ${CMAKE_SOURCE_DIR}/third_party
    )
    if(WIN32)
        target_include_directories(imgui_filedialog PUBLIC ${CMAKE_SOURCE_DIR}/third_party/dirent)
    endif()
    target_link_libraries(imgui_filedialog PUBLIC imgui)
    message(STATUS "ImGuiFileDialog: ENABLED")
    set(HAVE_IFD TRUE)
else()
    message(STATUS "ImGuiFileDialog: NOT FOUND -> stub mode")
    set(HAVE_IFD FALSE)
endif()

# ---------------- App ----------------
file(GLOB_RECURSE APP_SRC CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/src/*.cpp)
if(WIN32)
    add_executable(fax_decoder ${APP_SRC} resources/app.rc)
else()
    add_executable(fax_decoder ${APP_SRC})
endif()

target_include_directories(fax_decoder PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/third_party
    ${CMAKE_SOURCE_DIR}/third_party/dr_libs
    ${CMAKE_SOURCE_DIR}/third_party/stb
    ${CMAKE_SOURCE_DIR}/third_party/inih
    ${CMAKE_SOURCE_DIR}/third_party/json
    ${CMAKE_SOURCE_DIR}/third_party/kissfft
    ${GLFW_INCLUDE_DIRS}
)
if(WIN32)
    target_include_directories(fax_decoder PRIVATE ${CMAKE_SOURCE_DIR}/third_party/dirent)
endif()

target_link_libraries(fax_decoder PRIVATE
    imgui ${GLFW_LIBRARIES} Threads::Threads CURL::libcurl)

if(HAVE_IFD)
    target_link_libraries(fax_decoder PRIVATE imgui_filedialog)
    target_compile_definitions(fax_decoder PRIVATE HAVE_IMGUI_FILE_DIALOG=1)
endif()

if(PORTAUDIO_FOUND)
    target_include_directories(fax_decoder PRIVATE ${PORTAUDIO_INCLUDE_DIRS})
    target_link_libraries(fax_decoder PRIVATE ${PORTAUDIO_LIBRARIES})
    target_compile_definitions(fax_decoder PRIVATE HAVE_PORTAUDIO=1)
endif()

if(OpenGL_FOUND)
    target_link_libraries(fax_decoder PRIVATE OpenGL::GL)
else()
    target_link_libraries(fax_decoder PRIVATE GL)
endif()

target_sources(fax_decoder PRIVATE
    ${CMAKE_SOURCE_DIR}/third_party/inih/ini.c
    ${CMAKE_SOURCE_DIR}/third_party/kissfft/kiss_fft.c
    ${CMAKE_SOURCE_DIR}/third_party/kissfft/kiss_fftr.c
)

# ======== CPack 打包 (Windows NSIS 安装包) ========
if(WIN32)
    set(CPACK_GENERATOR "NSIS")
    set(CPACK_PACKAGE_NAME "FAXDecoder")
    set(CPACK_PACKAGE_VENDOR "78-HAM")
    set(CPACK_PACKAGE_VERSION "2.0.0")
    set(CPACK_NSIS_DISPLAY_NAME "FAX Decoder 2.0")
    set(CPACK_NSIS_MUI_ICON "${CMAKE_SOURCE_DIR}/resources/icon.ico") # 如果有图标
    set(CPACK_NSIS_CREATE_ICONS_EXTRA "CreateShortCut '$SMPROGRAMS\\\\FAX Decoder.lnk' '$INSTDIR\\\\fax_decoder.exe'")
    # 桌面快捷方式可选逻辑 (NSIS 默认不支持容易勾选，先默认创建)
    set(CPACK_NSIS_CREATE_ICONS_EXTRA "${CPACK_NSIS_CREATE_ICONS_EXTRA} CreateShortCut '$DESKTOP\\\\FAX Decoder.lnk' '$INSTDIR\\\\fax_decoder.exe'")
    
    include(CPack)
endif()
