#pragma once

#include <d3d12.h>
#include <wrl/client.h>
#include <memory>

namespace jisaku
{
    class DX12Device;
    class Swapchain;

    class RenderPass_Triangle
    {
    public:
        RenderPass_Triangle();
        ~RenderPass_Triangle();

        bool Initialize(DX12Device* device, Swapchain* swapchain);
        void Shutdown();
        void Execute(ID3D12GraphicsCommandList* cmd, Swapchain& swap);

    private:
        bool CreateRootSignature();
        bool CreatePipelineState();
        bool CreateVertexBuffer();

        DX12Device* m_device;
        Swapchain* m_swapchain;
        
        Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSignature;
        Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pipelineState;
        Microsoft::WRL::ComPtr<ID3D12Resource> m_vertexBuffer;
        D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;
    };
}
