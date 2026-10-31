cmake_minimum_required(VERSION 3.16)
project(fax_decoder LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Windows 平台专属：启用 RC 资源编译器以支持 exe 图标
if(WIN32)
    set(CPACK_GENERATOR "NSIS")
    set(CPACK_PACKAGE_NAME "FAXDecoder")
    set(CPACK_PACKAGE_VENDOR "78-HAM")
    set(CPACK_PACKAGE_VERSION "2.0.0")
    set(CPACK_PACKAGE_INSTALL_DIRECTORY "FAXDecoder")
    set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "FAX Decoder 2.0")
    
    set(CPACK_NSIS_DISPLAY_NAME "FAX Decoder 2.0")
    set(CPACK_NSIS_PACKAGE_NAME "FAX Decoder 2.0")
    
    set(CPACK_CREATE_DESKTOP_LINKS "fax_decoder")
    set(CPACK_NSIS_MENU_LINKS "fax_decoder.exe" "FAX Decoder")
    
    set(CPACK_NSIS_MUI_ICON "${CMAKE_SOURCE_DIR}/resources/icon.ico")
    set(CPACK_NSIS_MUI_UNIICON "${CMAKE_SOURCE_DIR}/resources/icon.ico")

    # 核心修改：让 CPack 直接打包 build/dist 文件夹里的所有内容
    set(CPACK_INSTALLED_DIRECTORIES "${CMAKE_BINARY_DIR}/dist;/" ${CPACK_INSTALLED_DIRECTORIES})

    include(CPack)
endif()
