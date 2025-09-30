#pragma once
#include <wrl.h>
#include <d3d12.h>
#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>
#include <memory>

namespace jisaku {

class DX12Device;

struct ShaderDesc {
    std::wstring hlslPath;
    std::wstring entryVS = L"VSMain";
    std::wstring entryPS = L"PSMain";
    std::wstring targetVS = L"vs_6_0";
    std::wstring targetPS = L"ps_6_0";
    std::vector<std::wstring> defines; // L"NAME=VALUE"
};

struct ShaderBlobs {
    Microsoft::WRL::ComPtr<ID3DBlob> vs;
    Microsoft::WRL::ComPtr<ID3DBlob> ps;
};

class IHotReloadable {
public:
    virtual ~IHotReloadable() = default;
    virtual void OnShadersReloaded(const ShaderBlobs& blobs) = 0; // PSO再作成を内部で行う
};

class ShaderReloader {
public:
    bool Init(DX12Device* dev);
    // 監視対象を登録。戻り値は id。
    int Register(const ShaderDesc& desc, IHotReloadable* sink);
    // 毎フレーム呼び出し。intervalSec 経過時に監視チェック→変更があれば再コンパイル＆sinkへ通知。
    void Tick(double dt, double intervalSec = 0.5);
    // 明示的に再ビルド（ImGuiボタンから呼ぶ）
    void ForceRebuildAll();

private:
    struct Item {
        ShaderDesc desc;
        IHotReloadable* sink = nullptr;
        std::filesystem::file_time_type lastWrite{};
        ShaderBlobs cached;
    };
    jisaku::DX12Device* m_dev = nullptr;
    std::unordered_map<int, Item> m_items;
    int m_nextId = 1;
    double m_accum = 0.0;

    bool compile_(const ShaderDesc& desc, ShaderBlobs& out, std::wstring& error);
    bool detectChanged_(Item& it);
};

} // namespace jisaku
