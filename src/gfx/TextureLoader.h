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

    private:
        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_srvHeap;
        UINT m_srvInc = 0;
        std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> m_pendingUploads;
    };
}
