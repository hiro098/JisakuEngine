#include "TextureLoader.h"
#include <d3d12.h>
#include <DirectXTex.h>
#include <spdlog/spdlog.h>
#include <vector>

namespace jisaku
{
    bool TextureLoader::Init(ID3D12Device* dev)
    {
        // SRV heap作成（1個のみ、shader-visible必須）
        D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
        srvHeapDesc.NumDescriptors = 1;
        srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

        HRESULT hr = dev->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&m_srvHeap));
        if (FAILED(hr))
        {
            spdlog::error("Failed to create SRV heap: 0x{:x}", hr);
            return false;
        }

        // ヒープがshader-visibleであることを確認
        auto desc = m_srvHeap->GetDesc();
        if ((desc.Flags & D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE) == 0)
        {
            spdlog::error("SRV heap is not shader-visible!");
            return false;
        }

        m_srvInc = dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        return true;
    }

    TextureHandle TextureLoader::CreateCheckerboard(ID3D12Device* dev, ID3D12GraphicsCommandList* cmd,
                                                    uint32_t size, uint32_t cell)
    {
        TextureHandle handle;

        // CPUでチェッカーボード生成（デバッグ用に色を変更）
        std::vector<uint8_t> pixels(size * size * 4);
        for (uint32_t y = 0; y < size; ++y)
        {
            for (uint32_t x = 0; x < size; ++x)
            {
                uint32_t cellX = x / cell;
                uint32_t cellY = y / cell;
                bool isWhite = (cellX + cellY) % 2 == 0;
                
                uint32_t index = (y * size + x) * 4;
                if (isWhite) {
                    pixels[index + 0] = 255; // R
                    pixels[index + 1] = 0;   // G
                    pixels[index + 2] = 0;   // B
                } else {
                    pixels[index + 0] = 0;   // R
                    pixels[index + 1] = 255; // G
                    pixels[index + 2] = 0;   // B
                }
                pixels[index + 3] = 255;   // A
            }
        }

        // テクスチャ用リソース記述（2D RGBA8）
        D3D12_RESOURCE_DESC texDesc = {};
        texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        texDesc.Alignment = 0;
        texDesc.Width = size;
        texDesc.Height = size;
        texDesc.DepthOrArraySize = 1;
        texDesc.MipLevels = 1;
        texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        texDesc.SampleDesc.Count = 1;
        texDesc.SampleDesc.Quality = 0;
        texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        texDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

        // Upload heap作成（バッファとして）
        D3D12_HEAP_PROPERTIES uploadHeapProps = {};
        uploadHeapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
        uploadHeapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        uploadHeapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        uploadHeapProps.CreationNodeMask = 1;
        uploadHeapProps.VisibleNodeMask = 1;

        // 必要なアップロードバッファサイズを計算（フットプリントも取得）
        UINT64 uploadBufferSize = 0;
        UINT numRows = 0;
        UINT64 rowSizeInBytes = 0;
        D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint = {};
        dev->GetCopyableFootprints(&texDesc, 0, 1, 0, &footprint, &numRows, &rowSizeInBytes, &uploadBufferSize);

        D3D12_RESOURCE_DESC uploadDesc = {};
        uploadDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        uploadDesc.Alignment = 0;
        uploadDesc.Width = uploadBufferSize;
        uploadDesc.Height = 1;
        uploadDesc.DepthOrArraySize = 1;
        uploadDesc.MipLevels = 1;
        uploadDesc.Format = DXGI_FORMAT_UNKNOWN;
        uploadDesc.SampleDesc.Count = 1;
        uploadDesc.SampleDesc.Quality = 0;
        uploadDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        uploadDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

        Microsoft::WRL::ComPtr<ID3D12Resource> uploadResource;
        HRESULT hr = dev->CreateCommittedResource(
            &uploadHeapProps,
            D3D12_HEAP_FLAG_NONE,
            &uploadDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&uploadResource)
        );
        if (FAILED(hr))
        {
            spdlog::error("Failed to create upload resource: 0x{:x}", hr);
            return handle;
        }

        // アップロードリソースを未解放リストに追加
        m_pendingUploads.push_back(uploadResource);

        // Default heap作成（テクスチャ）
        D3D12_HEAP_PROPERTIES defaultHeapProps = {};
        defaultHeapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
        defaultHeapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        defaultHeapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        defaultHeapProps.CreationNodeMask = 1;
        defaultHeapProps.VisibleNodeMask = 1;

        hr = dev->CreateCommittedResource(
            &defaultHeapProps,
            D3D12_HEAP_FLAG_NONE,
            &texDesc,
            D3D12_RESOURCE_STATE_COMMON,
            nullptr,
            IID_PPV_ARGS(&handle.resource)
        );
        if (FAILED(hr))
        {
            spdlog::error("Failed to create default resource: 0x{:x}", hr);
            return handle;
        }
        spdlog::info("Default texture resource created with COMMON state");

        // データをアップロード（デバッグ用にログ出力）
        D3D12_SUBRESOURCE_DATA subresourceData = {};
        subresourceData.pData = pixels.data();
        subresourceData.RowPitch = size * 4;
        subresourceData.SlicePitch = size * size * 4;

        spdlog::info("Uploading texture: {}x{}, RowPitch={}, SlicePitch={}", 
                     size, size, subresourceData.RowPitch, subresourceData.SlicePitch);
        spdlog::info("Footprint: Offset={}, RowPitch={}, Width={}, Height={}", 
                     footprint.Offset, footprint.Footprint.RowPitch, 
                     footprint.Footprint.Width, footprint.Footprint.Height);

        UINT8* pData;
        HRESULT mapHr = uploadResource->Map(0, nullptr, reinterpret_cast<void**>(&pData));
        if (SUCCEEDED(mapHr))
        {
            for (UINT row = 0; row < numRows; ++row)
            {
                // フットプリントの先頭オフセットを考慮
                memcpy(pData + footprint.Offset + row * footprint.Footprint.RowPitch, 
                       pixels.data() + row * subresourceData.RowPitch, 
                       subresourceData.RowPitch);
            }
            uploadResource->Unmap(0, nullptr);
            spdlog::info("Texture data uploaded successfully");
        }
        else
        {
            spdlog::error("Failed to map upload resource: 0x{:x}", mapHr);
        }

        D3D12_TEXTURE_COPY_LOCATION src = {};
        src.pResource = uploadResource.Get();
        src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
        src.PlacedFootprint = footprint;

        D3D12_TEXTURE_COPY_LOCATION dst = {};
        dst.pResource = handle.resource.Get();
        dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        dst.SubresourceIndex = 0;

        // COMMON -> COPY_DEST に遷移
        D3D12_RESOURCE_BARRIER toCopyDest = {};
        toCopyDest.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        toCopyDest.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        toCopyDest.Transition.pResource = handle.resource.Get();
        toCopyDest.Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
        toCopyDest.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
        toCopyDest.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        cmd->ResourceBarrier(1, &toCopyDest);
        spdlog::info("Resource barrier recorded: COMMON -> COPY_DEST");

        spdlog::info("Copying texture region from upload to default heap");
        cmd->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);
        spdlog::info("Texture copy command recorded");

        // Copy dest -> shader resource
        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barrier.Transition.pResource = handle.resource.Get();
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        cmd->ResourceBarrier(1, &barrier);
        spdlog::info("Resource barrier recorded: COPY_DEST -> PIXEL_SHADER_RESOURCE");

        // SRV作成（フォーマットはリソースと同じUNORM）
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = 1;
        srvDesc.Texture2D.MostDetailedMip = 0;
        srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

        // SRVをヒープの先頭に作成
        handle.srvCPU = m_srvHeap->GetCPUDescriptorHandleForHeapStart();
        handle.srvGPU = m_srvHeap->GetGPUDescriptorHandleForHeapStart();
        dev->CreateShaderResourceView(handle.resource.Get(), &srvDesc, handle.srvCPU);
        spdlog::info("SRV created at CPU handle: {}, GPU handle: {}", 
                     handle.srvCPU.ptr, handle.srvGPU.ptr);

        return handle;
    }

    ID3D12DescriptorHeap* TextureLoader::GetSrvHeap() const
    {
        return m_srvHeap.Get();
    }

    void TextureLoader::FlushUploads()
    {
        m_pendingUploads.clear();
    }

}
