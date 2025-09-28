#pragma once

#include <d3d12.h>
#include <wrl/client.h>
#include <Windows.h>

namespace jisaku
{
    class DX12Device;
    class Swapchain;

    class ImGuiLayer
    {
    public:
        ImGuiLayer();
        ~ImGuiLayer();

        bool Initialize(DX12Device* device, Swapchain* swapchain, HWND hwnd);
        void Shutdown();
        void Update();
        void Render();

    private:
        bool CreateDescriptorHeap();
        void SetupImGuiStyle();

        DX12Device* m_device;
        Swapchain* m_swapchain;
        HWND m_hwnd;
        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_srvHeap;
        UINT m_srvDescriptorSize;
        bool m_vsyncEnabled;
        float m_fps;
        float m_frameTime;
    };
}
