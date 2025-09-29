#pragma once

#include <d3d12.h>
#include <wrl/client.h>
#include <memory>
#include "TextureLoader.h"

namespace jisaku
{
    class DX12Device;
    class Swapchain;

    class RenderPass_TexturedQuad
    {
    public:
        RenderPass_TexturedQuad();
        ~RenderPass_TexturedQuad();

        bool Initialize(DX12Device* device, Swapchain* swapchain);
        void Execute(ID3D12GraphicsCommandList* cmd, Swapchain& swap);

    private:
        DX12Device* m_device;
        Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSignature;
        Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pipelineState;
        Microsoft::WRL::ComPtr<ID3D12Resource> m_vertexBuffer;
        Microsoft::WRL::ComPtr<ID3D12Resource> m_indexBuffer;
        D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;
        D3D12_INDEX_BUFFER_VIEW m_indexBufferView;
        std::unique_ptr<TextureLoader> m_textureLoader;
        TextureHandle m_texture;
    };
}
