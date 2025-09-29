#include "gfx/GPUTimer.h"
#include "gfx/DX12Device.h"
#include <imgui.h>

using Microsoft::WRL::ComPtr;
using namespace jisaku;

static UINT64 AlignTo(UINT64 v, UINT64 align) { return (v + (align - 1)) & ~(align - 1); }

bool GPUTimer::Init(DX12Device* dev, UINT maxSamplesPerFrame) {
    m_dev = dev;
    m_frameCount = dev->GetFrameCount();
    m_maxSamplesPerFrame = maxSamplesPerFrame;

    D3D12_QUERY_HEAP_DESC qh{};
    qh.Type = D3D12_QUERY_HEAP_TYPE_TIMESTAMP;
    qh.Count = m_frameCount * (m_maxSamplesPerFrame * 2);
    if (FAILED(dev->GetDevice()->CreateQueryHeap(&qh, IID_PPV_ARGS(&m_queryHeap)))) return false;

    m_frames.resize(m_frameCount);
    const UINT64 sizePerFrame = sizeof(UINT64) * (m_maxSamplesPerFrame * 2);
    for (auto& f : m_frames) {
        D3D12_RESOURCE_DESC rdDesc{};
        rdDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        rdDesc.Alignment = 0;
        rdDesc.Width = AlignTo(sizePerFrame, 256);
        rdDesc.Height = 1;
        rdDesc.DepthOrArraySize = 1;
        rdDesc.MipLevels = 1;
        rdDesc.Format = DXGI_FORMAT_UNKNOWN;
        rdDesc.SampleDesc.Count = 1;
        rdDesc.SampleDesc.Quality = 0;
        rdDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        rdDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

        D3D12_HEAP_PROPERTIES heapProps{};
        heapProps.Type = D3D12_HEAP_TYPE_READBACK;
        heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        heapProps.CreationNodeMask = 1;
        heapProps.VisibleNodeMask = 1;

        if (FAILED(dev->GetDevice()->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &rdDesc,
            D3D12_RESOURCE_STATE_COPY_DEST,
            nullptr,
            IID_PPV_ARGS(f.readback.ReleaseAndGetAddressOf())))) return false;
    }

    dev->GetQueue()->GetTimestampFrequency(&m_freq);
    return true;
}

void GPUTimer::NewFrame() {
    m_frameIndex = m_dev->GetFrameIndex();
    auto& f = m_frames[m_frameIndex];
    f.cursor = 0;
    f.usedQueries = 0;
    for (auto& kv : f.samples) {
        kv.second.queryBegin = UINT_MAX;
        kv.second.queryEnd = UINT_MAX;
    }
}

GPUTimer::Sample& GPUTimer::allocSample_(const char* name) {
    auto& f = m_frames[m_frameIndex];
    auto it = f.samples.find(name);
    if (it == f.samples.end()) it = f.samples.emplace(name, Sample{}).first;
    auto& s = it->second;
    if (s.queryBegin == UINT_MAX) { s.queryBegin = f.cursor++; s.queryEnd = f.cursor++; }
    return s;
}

void GPUTimer::Begin(ID3D12GraphicsCommandList* cmd, const char* name) {
    auto& f = m_frames[m_frameIndex];
    if (f.cursor + 2 > m_maxSamplesPerFrame * 2) return;
    auto& s = allocSample_(name);
    const UINT base = m_frameIndex * (m_maxSamplesPerFrame * 2);
    cmd->EndQuery(m_queryHeap.Get(), D3D12_QUERY_TYPE_TIMESTAMP, base + s.queryBegin);
}

void GPUTimer::End(ID3D12GraphicsCommandList* cmd, const char* name) {
    auto& f = m_frames[m_frameIndex];
    auto& s = allocSample_(name);
    const UINT base = m_frameIndex * (m_maxSamplesPerFrame * 2);
    cmd->EndQuery(m_queryHeap.Get(), D3D12_QUERY_TYPE_TIMESTAMP, base + s.queryEnd);
    f.usedQueries = (std::max)(f.usedQueries, s.queryEnd + 1);
}

void GPUTimer::Resolve(ID3D12GraphicsCommandList* cmd) {
    auto& f = m_frames[m_frameIndex];
    if (f.usedQueries == 0) return;
    const UINT base = m_frameIndex * (m_maxSamplesPerFrame * 2);
    cmd->ResolveQueryData(m_queryHeap.Get(), D3D12_QUERY_TYPE_TIMESTAMP, base, f.usedQueries, f.readback.Get(), 0);
}

void GPUTimer::Collect() {
    const UINT prev = (m_frameIndex + m_frameCount - 1) % m_frameCount;
    auto& f = m_frames[prev];
    if (f.usedQueries == 0) return;
    UINT64* data = nullptr;
    if (SUCCEEDED(f.readback->Map(0, nullptr, reinterpret_cast<void**>(&data))) && data) {
        for (auto& kv : f.samples) {
            auto& name = kv.first;
            auto& s = kv.second;
            if (s.queryBegin == UINT_MAX || s.queryEnd == UINT_MAX) continue;
            const UINT64 t0 = data[s.queryBegin];
            const UINT64 t1 = data[s.queryEnd];
            if (t1 > t0) {
                s.ms = (double)(t1 - t0) * 1000.0 / (double)m_freq;
                s.avgMs = (s.hits == 0) ? s.ms : (s.avgMs * 0.9 + s.ms * 0.1);
                s.hits++;
            }
        }
        f.readback->Unmap(0, nullptr);
    }
}

void GPUTimer::DrawImGui() {
    if (ImGui::Begin("Profiler")) {
        ImGui::Text("GPU timings (ms)");
        const UINT prev = (m_frameIndex + m_frameCount - 1) % m_frameCount;
        auto& f = m_frames[prev];
        for (auto& kv : f.samples) {
            const auto& name = kv.first;
            const auto& s = kv.second;
            ImGui::Text("%s  cur: %.3f  avg: %.3f", name.c_str(), s.ms, s.avgMs);
        }
    }
    ImGui::End();
}


