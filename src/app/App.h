#pragma once

#include <Windows.h>
#include <memory>
#include <wrl/client.h>
#include "ui/ImGuiLayer.h"

namespace jisaku
{
    class DX12Device;
    class Swapchain;
    class RenderPass_Clear;
    class RenderPass_Triangle;

    class App
    {
    public:
        App();
        ~App();

        bool Initialize(HINSTANCE hInstance);
        int Run();

    private:
        static LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
        void Update();
        void Render();

        HINSTANCE m_hInstance;
        HWND m_hwnd;
        bool m_running;

        std::unique_ptr<DX12Device> m_device;
        std::unique_ptr<Swapchain> m_swapchain;
        ImGuiLayer m_imgui;
        std::unique_ptr<RenderPass_Clear> m_renderPass;
        std::unique_ptr<RenderPass_Triangle> m_trianglePass;
    };
}
