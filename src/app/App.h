#pragma once

#include <Windows.h>
#include <memory>
#include <wrl/client.h>

namespace jisaku
{
    class DX12Device;
    class Swapchain;
    class ImGuiLayer;
    class RenderPass_Clear;

    class App
    {
    public:
        App();
        ~App();

        bool Initialize(HINSTANCE hInstance);
        int Run();

    private:
        static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
        void Update();
        void Render();

        HINSTANCE m_hInstance;
        HWND m_hwnd;
        bool m_running;

        std::unique_ptr<DX12Device> m_device;
        std::unique_ptr<Swapchain> m_swapchain;
        std::unique_ptr<ImGuiLayer> m_imgui;
        std::unique_ptr<RenderPass_Clear> m_renderPass;
    };
}
