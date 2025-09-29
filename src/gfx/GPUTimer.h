#pragma once

#include <wrl/client.h>
#include <d3d12.h>
#include <string>
#include <vector>
#include <unordered_map>

namespace jisaku {

class DX12Device;

class GPUTimer {
public:
    struct Sample {
        UINT queryBegin = UINT_MAX;
        UINT queryEnd   = UINT_MAX;
        double ms = 0.0;
        double avgMs = 0.0;
        int hits = 0;
    };

    bool Init(DX12Device* dev, UINT maxSamplesPerFrame = 32);
    void NewFrame();
    void Begin(ID3D12GraphicsCommandList* cmd, const char* name);
    void End(ID3D12GraphicsCommandList* cmd, const char* name);
    void Resolve(ID3D12GraphicsCommandList* cmd);
    void Collect();
    void DrawImGui();

private:
    struct FrameBuf {
        Microsoft::WRL::ComPtr<ID3D12Resource> readback;
        UINT usedQueries = 0;
        std::unordered_map<std::string, Sample> samples;
        UINT cursor = 0;
    };

    DX12Device* m_dev = nullptr;
    Microsoft::WRL::ComPtr<ID3D12QueryHeap> m_queryHeap;
    UINT m_frameCount = 0;
    UINT m_maxSamplesPerFrame = 0; // Begin/Endで×2クエリ
    UINT64 m_freq = 0;

    std::vector<FrameBuf> m_frames;
    UINT m_frameIndex = 0;

    Sample& allocSample_(const char* name);
};

} // namespace jisaku


