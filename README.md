# JisakuEngine

C++/DirectX 12を使用した自作ゲームエンジンです。

## 要件

- Windows 10/11
- Visual Studio 2019/2022 (C++20対応)
- CMake 3.20以上
- vcpkg
- DirectX 12 Agility SDK

## ビルド手順

### 1. vcpkgのセットアップ

```bash
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
./bootstrap-vcpkg.bat
```

### 2. プロジェクトのビルド

```bash
mkdir build
cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=../vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build . --config Release
```

### 3. 実行

```bash
./Release/JisakuEngine.exe
```

## 機能

- DirectX 12によるレンダリング
- ImGuiによるデバッグUI
- FPS表示
- VSyncトグル
- 青い背景のクリア画面

## プロジェクト構成

```
JisakuEngine/
├── CMakeLists.txt          # CMake設定
├── vcpkg.json              # vcpkg依存関係
├── cmake/
│   └── FindDXC.cmake      # DXC検索
├── src/
│   ├── main.cpp           # エントリーポイント
│   ├── app/               # アプリケーション層
│   ├── gfx/               # グラフィックス層
│   └── ui/                # UI層
├── shaders/
│   └── fullscreen.hlsl    # シェーダー
└── README.md
```

## 注意事項

- Windowsでの実行を前提としています
- DirectX 12対応のGPUが必要です
- デバッグビルドではDirectX 12 Debug Layerが有効になります
