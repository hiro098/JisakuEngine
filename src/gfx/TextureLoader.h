#pragma once

#include <d3d12.h>
#include <wrl/client.h>
#include <cstdint>
#include <vector>
#include <string>

namespace jisaku
{
    struct TextureHandle
    {
        Microsoft::WRL::ComPtr<ID3D12Resource> resource; // default heap
        D3D12_CPU_DESCRIPTOR_HANDLE srvCPU{};
        D3D12_GPU_DESCRIPTOR_HANDLE srvGPU{};
        uint32_t slot = UINT32_MAX; // SRVヒープ内スロット
    };

    class TextureLoader
    {
    public:
        bool Init(ID3D12Device* dev);
        TextureHandle CreateCheckerboard(ID3D12Device* dev, ID3D12GraphicsCommandList* cmd,
                                         uint32_t size = 256, uint32_t cell = 32);
        
        // 追加: 外部画像ファイルを読み込んで out にSRVを上書き
        bool LoadFromFile(ID3D12Device* dev,
                          ID3D12GraphicsCommandList* cmd,
                          const std::wstring& path,
                          /*inout*/ TextureHandle& out,
                          bool forceSRGB = true,
                          bool generateMips = true);

        ID3D12DescriptorHeap* GetSrvHeap() const;
        void FlushUploads();

        // 追加API
        uint32_t GetDescriptorSize() const { return m_srvInc; }
        uint32_t GetCapacity() const { return m_capacity; }
        bool     IsValidSlot(uint32_t s) const { return s < m_capacity; }

    private:
        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_srvHeap;
        UINT m_srvInc = 0;
        uint32_t m_capacity = 0;
        std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> m_pendingUploads;
        std::vector<bool> m_used;

        uint32_t AllocateSlot_();
        D3D12_CPU_DESCRIPTOR_HANDLE CpuHandleOf_(uint32_t slot) const;
        D3D12_GPU_DESCRIPTOR_HANDLE GpuHandleOf_(uint32_t slot) const;
    };
}
