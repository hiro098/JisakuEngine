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

    void RenderPass_Clear::Execute()
    {
        // コマンドアロケーターをリセット
        m_device->GetCommandAllocator()->Reset();

        // コマンドリストをリセット
        m_device->GetCommandList()->Reset(m_device->GetCommandAllocator(), nullptr);

        // リソースバリア設定（レンダーターゲットへの遷移）
        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barrier.Transition.pResource = m_swapchain->GetBackBuffer(m_swapchain->GetCurrentBackBufferIndex());
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

        m_device->GetCommandList()->ResourceBarrier(1, &barrier);

        // レンダーターゲットを設定
        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_swapchain->GetRTVHandle(m_swapchain->GetCurrentBackBufferIndex());
        m_device->GetCommandList()->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

        // バックバッファを青でクリア
        const float clearColor[] = { 0.392f, 0.584f, 0.929f, 1.0f };
        m_device->GetCommandList()->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);

        // リソースバリア設定（プレゼントへの遷移）
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;

        m_device->GetCommandList()->ResourceBarrier(1, &barrier);

        // コマンドリストを閉じる
        m_device->GetCommandList()->Close();

        // コマンドを実行
        ID3D12CommandList* commandLists[] = { m_device->GetCommandList() };
        m_device->GetCommandQueue()->ExecuteCommandLists(1, commandLists);
    }
}
