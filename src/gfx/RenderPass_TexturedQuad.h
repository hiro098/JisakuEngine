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
        void SetTexture(const TextureHandle& h);
        void SetActiveSlot(uint32_t slot);
        uint32_t GetActiveSlot() const { return m_activeSlot; }
        TextureLoader* GetTextureLoader() const { return m_textureLoader.get(); }

    private:
        DX12Device* m_device;
        Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSignature;
        Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pipelineState;
        Microsoft::WRL::ComPtr<ID3D12Resource> m_cbUpload;
        D3D12_GPU_VIRTUAL_ADDRESS m_cbGpuVA = 0;
        size_t m_cbOffset = 0;
        Microsoft::WRL::ComPtr<ID3D12Resource> m_vertexBuffer;
        Microsoft::WRL::ComPtr<ID3D12Resource> m_indexBuffer;
        D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;
        D3D12_INDEX_BUFFER_VIEW m_indexBufferView;
        std::unique_ptr<TextureLoader> m_textureLoader;
        TextureHandle m_texture;
        uint32_t m_activeSlot = UINT32_MAX;
        float m_transX = 0.0f;
        float m_transY = 0.0f;
        float m_rotDeg = 0.0f;
        float m_scaleX = 1.0f;
        float m_scaleY = 1.0f;

    public:
        void SetTransform(float tx, float ty, float rotDeg, float sx, float sy) {
            m_transX = tx; m_transY = ty; m_rotDeg = rotDeg; m_scaleX = sx; m_scaleY = sy;
        }
    };
}
