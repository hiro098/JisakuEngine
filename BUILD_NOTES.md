# JisakuEngine ビルド手順

## Windows での再構成・ビルド手順

### 1. 既存ビルドのクリーンアップ
```powershell
rmdir build -Recurse -Force
```

### 2. CMake 構成
```cmd
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 ^
  -DCMAKE_TOOLCHAIN_FILE="%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake" ^
  -DVCPKG_TARGET_TRIPLET=x64-windows ^
  -DDXC_EXECUTABLE="C:/Program Files (x86)/Windows Kits/10/bin/10.0.19041.0/x64/dxc.exe"
```

### 3. ビルド実行
```cmd
cmake --build build --config Debug
```

### 4. 実行
```cmd
.\build\Debug\JisakuEngine.exe
```

## 注意事項
- VCPKG_ROOT 環境変数が設定されていることを確認してください
- Windows SDK がインストールされていることを確認してください
- DirectX 12 対応の GPU が必要です
