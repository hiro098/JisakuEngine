#include "RenderPass_Clear.h"
#include "DX12Device.h"
#include "Swapchain.h"
#include <d3d12.h>
#include <spdlog/spdlog.h>

namespace jisaku
{
    RenderPass_Clear::RenderPass_Clear() : m_device(nullptr), m_swapchain(nullptr)
    {
    }

    RenderPass_Clear::~RenderPass_Clear()
    {
    }

    bool RenderPass_Clear::Initialize(DX12Device* device, Swapchain* swapchain)
    {
        m_device = device;
        m_swapchain = swapchain;

        spdlog::info("RenderPass_Clear initialized successfully");
        return true;
    }

    void RenderPass_Clear::Execute(ID3D12GraphicsCommandList* cmd, Swapchain& swap, const float clear[4])
    {
        // Present -> RenderTarget
        auto back = swap.GetCurrentBackBuffer();
        D3D12_RESOURCE_BARRIER toRT = {};
        toRT.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        toRT.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        toRT.Transition.pResource = back;
        toRT.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
        toRT.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
        toRT.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        cmd->ResourceBarrier(1, &toRT);

        auto rtv = swap.GetCurrentRTV();
        cmd->OMSetRenderTargets(1, &rtv, FALSE, nullptr);
        D3D12_VIEWPORT vp{ 0.0f, 0.0f, (float)swap.GetWidth(), (float)swap.GetHeight(), 0.0f, 1.0f };
        D3D12_RECT sc{ 0, 0, (LONG)swap.GetWidth(), (LONG)swap.GetHeight() };
        cmd->RSSetViewports(1, &vp);
        cmd->RSSetScissorRects(1, &sc);

        cmd->ClearRenderTargetView(rtv, clear, 0, nullptr);

        // RenderTarget -> Present
        D3D12_RESOURCE_BARRIER toPresent = {};
        toPresent.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        toPresent.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        toPresent.Transition.pResource = back;
        toPresent.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
        toPresent.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
        toPresent.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        cmd->ResourceBarrier(1, &toPresent);
    }
}
