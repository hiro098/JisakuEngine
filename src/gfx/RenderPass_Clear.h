#pragma once

#include <d3d12.h>
#include <wrl/client.h>

namespace jisaku
{
    class DX12Device;
    class Swapchain;

    class RenderPass_Clear
    {
    public:
        RenderPass_Clear();
        ~RenderPass_Clear();

        bool Initialize(DX12Device* device, Swapchain* swapchain);
        void Execute();

    private:
        DX12Device* m_device;
        Swapchain* m_swapchain;
    };
}
