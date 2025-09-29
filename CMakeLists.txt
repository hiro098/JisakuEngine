cmake_minimum_required(VERSION 3.20)
project(JisakuEngine VERSION 0.1.0 LANGUAGES CXX)

# C++20設定
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# コンパイラ警告設定
if(MSVC)
    add_compile_options(/W4 /permissive- /EHsc)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# vcpkg設定
set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain file")
set(VCPKG_TARGET_TRIPLET "x64-windows" CACHE STRING "Vcpkg target triplet")

# DirectX 12 Agility SDK設定
set(D3D12SDKVersion "1.610.4" CACHE STRING "DirectX 12 Agility SDK version")
set(D3D12SDKPath "${CMAKE_CURRENT_SOURCE_DIR}/external/DirectX-Headers" CACHE STRING "DirectX 12 Agility SDK path")

# CMakeモジュールパス追加
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# DXC (DirectX Shader Compiler) – find by path or typical locations
if(NOT DEFINED DXC_EXECUTABLE)
  set(_DXC_HINTS
    "$ENV{DXC_EXECUTABLE}"
    "C:/ProgramData/chocolatey/bin"
    "C:/Program Files (x86)/Windows Kits/10/bin/10.0.22621.0/x64"
    "C:/Program Files (x86)/Windows Kits/10/bin/10.0.19041.0/x64"
    "C:/Program Files (x86)/Windows Kits/10/bin/10.0.18362.0/x64"
  )
  find_program(DXC_EXECUTABLE NAMES dxc HINTS ${_DXC_HINTS})
endif()

if(NOT DXC_EXECUTABLE)
  message(FATAL_ERROR "dxc.exe not found. Set -DDXC_EXECUTABLE=FULL_PATH_TO_dxc.exe")
else()
  message(STATUS "Using DXC: ${DXC_EXECUTABLE}")
endif()

# ソースファイル
set(SOURCES
    src/main.cpp
    src/app/App.cpp
    src/gfx/DX12Device.cpp
    src/gfx/Swapchain.cpp
    src/gfx/RenderPass_Clear.cpp
    src/gfx/RenderPass_Triangle.cpp
    src/gfx/RenderPass_TexturedQuad.cpp
    src/gfx/TextureLoader.cpp
    src/ui/ImGuiLayer.cpp
)

# ヘッダーファイル
set(HEADERS
    src/app/App.h
    src/gfx/DX12Device.h
    src/gfx/Swapchain.h
    src/gfx/RenderPass_Clear.h
    src/gfx/RenderPass_Triangle.h
    src/gfx/RenderPass_TexturedQuad.h
    src/gfx/TextureLoader.h
    src/ui/ImGuiLayer.h
)

# シェーダーコンパイル
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/shaders/fullscreen.cso
    COMMAND ${DXC_EXECUTABLE} -T ps_6_0 -E PSMain -Fo ${CMAKE_CURRENT_SOURCE_DIR}/shaders/fullscreen.cso ${CMAKE_CURRENT_SOURCE_DIR}/shaders/fullscreen.hlsl
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/shaders/fullscreen.hlsl
    COMMENT "Compiling fullscreen.hlsl"
)

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/shaders/Triangle.cso
    COMMAND ${DXC_EXECUTABLE} -T vs_6_0 -E VSMain -T ps_6_0 -E PSMain -Fo ${CMAKE_CURRENT_SOURCE_DIR}/shaders/Triangle.cso ${CMAKE_CURRENT_SOURCE_DIR}/shaders/Triangle.hlsl
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/shaders/Triangle.hlsl
    COMMENT "Compiling Triangle.hlsl"
)

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/shaders/TexturedQuad.cso
    COMMAND ${DXC_EXECUTABLE} -T vs_6_0 -E VSMain -T ps_6_0 -E PSMain -Fo ${CMAKE_CURRENT_SOURCE_DIR}/shaders/TexturedQuad.cso ${CMAKE_CURRENT_SOURCE_DIR}/shaders/TexturedQuad.hlsl
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/shaders/TexturedQuad.hlsl
    COMMENT "Compiling TexturedQuad.hlsl"
)

# 実行ファイル作成
add_executable(${PROJECT_NAME} WIN32 ${SOURCES} ${HEADERS})

# インクルードディレクトリ
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/app
    ${CMAKE_SOURCE_DIR}/src/gfx
    ${CMAKE_SOURCE_DIR}/src/ui
    ${D3D12SDKPath}/include
)

# Unicode定義
target_compile_definitions(${PROJECT_NAME} PRIVATE UNICODE _UNICODE)

# ライブラリリンク
find_package(fmt CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(directxtex CONFIG REQUIRED)
find_package(directxtk12 CONFIG REQUIRED)

target_link_libraries(${PROJECT_NAME} PRIVATE
    fmt::fmt
    spdlog::spdlog
    imgui::imgui
    Microsoft::DirectXTex
    Microsoft::DirectXTK12
    d3d12
    dxgi
    d3dcompiler
    ole32
    oleaut32
)

# シェーダーファイルをビルドに含める
add_custom_target(shaders ALL DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/fullscreen.cso
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/Triangle.cso
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/TexturedQuad.cso
)

# Agility SDK DLLをコピー（存在する場合のみ）
if(EXISTS "${D3D12SDKPath}/bin/x64/d3d12.dll")
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${D3D12SDKPath}/bin/x64/d3d12.dll"
        $<TARGET_FILE_DIR:${PROJECT_NAME}>
        COMMENT "Copying DirectX 12 Agility SDK DLL"
    )
endif()

# デバッグ情報設定
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(${PROJECT_NAME} PRIVATE _DEBUG)
endif()
