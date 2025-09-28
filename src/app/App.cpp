#include "App.h"
#include "gfx/DX12Device.h"
#include "gfx/Swapchain.h"
#include "gfx/RenderPass_Clear.h"
#include "ui/ImGuiLayer.h"
#include <spdlog/spdlog.h>

namespace jisaku
{
    App::App() : m_hInstance(nullptr), m_hwnd(nullptr), m_running(false)
    {
    }

    App::~App()
    {
    }

    bool App::Initialize(HINSTANCE hInstance)
    {
        m_hInstance = hInstance;

        // ウィンドウクラス登録
        WNDCLASSEX wc = {};
        wc.cbSize = sizeof(WNDCLASSEX);
        wc.style = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc = WindowProc;
        wc.hInstance = hInstance;
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        wc.lpszClassName = L"JisakuEngineWindow";

        if (!RegisterClassEx(&wc))
        {
            spdlog::error("Failed to register window class");
            return false;
        }

        // ウィンドウ作成
        m_hwnd = CreateWindowEx(
            0,
            L"JisakuEngineWindow",
            L"JisakuEngine",
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT, CW_USEDEFAULT,
            1280, 720,
            nullptr, nullptr,
            hInstance, this
        );

        if (!m_hwnd)
        {
            spdlog::error("Failed to create window");
            return false;
        }

        ShowWindow(m_hwnd, SW_SHOW);
        UpdateWindow(m_hwnd);

        // DX12Device初期化
        m_device = std::make_unique<DX12Device>();
        if (!m_device->Initialize())
        {
            spdlog::error("Failed to initialize DX12Device");
            return false;
        }

        // Swapchain初期化
        m_swapchain = std::make_unique<Swapchain>();
        if (!m_swapchain->Initialize(m_device.get(), m_hwnd))
        {
            spdlog::error("Failed to initialize Swapchain");
            return false;
        }

        // ImGui初期化
        m_imgui = std::make_unique<ImGuiLayer>();
        if (!m_imgui->Initialize(m_device.get(), m_swapchain.get(), m_hwnd))
        {
            spdlog::error("Failed to initialize ImGui");
            return false;
        }

        // RenderPass初期化
        m_renderPass = std::make_unique<RenderPass_Clear>();
        if (!m_renderPass->Initialize(m_device.get(), m_swapchain.get()))
        {
            spdlog::error("Failed to initialize RenderPass_Clear");
            return false;
        }

        m_running = true;
        spdlog::info("Application initialized successfully");
        return true;
    }

    int App::Run()
    {
        MSG msg = {};
        while (m_running)
        {
            while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
            {
                if (msg.message == WM_QUIT)
                {
                    m_running = false;
                    break;
                }

                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }

            if (m_running)
            {
                Update();
                Render();
            }
        }

        return static_cast<int>(msg.wParam);
    }

    LRESULT CALLBACK App::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        App* app = nullptr;

        if (uMsg == WM_NCCREATE)
        {
            CREATESTRUCT* cs = reinterpret_cast<CREATESTRUCT*>(lParam);
            app = static_cast<App*>(cs->lpCreateParams);
            SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(app));
        }
        else
        {
            app = reinterpret_cast<App*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
        }

        if (app)
        {
            switch (uMsg)
            {
            case WM_DESTROY:
                app->m_running = false;
                PostQuitMessage(0);
                return 0;

            case WM_SIZE:
                if (wParam != SIZE_MINIMIZED)
                {
                    // リサイズ処理
                }
                return 0;
            }
        }

        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }

    void App::Update()
    {
        m_imgui->Update();
    }

    void App::Render()
    {
        m_renderPass->Execute();
        m_imgui->Render();
        m_swapchain->Present();
    }
}
