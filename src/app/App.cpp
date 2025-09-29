#include "App.h"
#include "gfx/DX12Device.h"
#include "gfx/Swapchain.h"
#include "gfx/RenderPass_Clear.h"
#include "gfx/RenderPass_Triangle.h"
#include "gfx/RenderPass_TexturedQuad.h"
#include "gfx/TextureLoader.h"
#include "ui/ImGuiLayer.h"
#include "imgui_impl_win32.h"
#include <spdlog/spdlog.h>
#include <commdlg.h>
#include <imgui.h>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace jisaku
{
    App::App() : m_hInstance(nullptr), m_hwnd(nullptr), m_running(false), m_inSizeMove(false)
    {
    }

    App::~App()
    {
    }

    bool App::Initialize(HINSTANCE hInstance)
    {
        m_hInstance = hInstance;

        // ウィンドウクラス登録
        WNDCLASSEXW wc{ sizeof(WNDCLASSEXW) };
        wc.style = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc = &jisaku::App::WndProc;
        wc.hInstance = hInstance;
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        wc.lpszClassName = L"JisakuWindowClass";

        if (!RegisterClassExW(&wc))
        {
            spdlog::error("Failed to register window class");
            return false;
        }

        // ウィンドウ作成
        m_hwnd = CreateWindowExW(
            0, L"JisakuWindowClass", L"JisakuEngine",
            WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 1280, 720,
            nullptr, nullptr, hInstance, this
        );

        if (!m_hwnd)
        {
            spdlog::error("Failed to create window");
            return false;
        }

        // ウィンドウを表示
        ShowWindow(m_hwnd, SW_SHOW);
        UpdateWindow(m_hwnd);
        
        // メッセージを処理してウィンドウを完全に初期化
        MSG msg;
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        // DX12Device初期化
        m_device = std::make_unique<DX12Device>();
        if (!m_device->Initialize())
        {
            spdlog::error("Failed to initialize DX12Device");
            return false;
        }
        // アップロード専用コンテキスト初期化
        m_device->InitUploadContext();

        // スワップチェーン初期化
        m_swapchain = std::make_unique<Swapchain>();
        if (!m_swapchain->Initialize(m_device.get(), m_hwnd))
        {
            spdlog::error("Failed to initialize Swapchain");
            return false;
        }

        // RenderPass初期化
        m_renderPass = std::make_unique<RenderPass_Clear>();
        if (!m_renderPass->Initialize(m_device.get(), m_swapchain.get()))
        {
            spdlog::error("Failed to initialize RenderPass_Clear");
            return false;
        }

        // TrianglePass初期化
        m_trianglePass = std::make_unique<RenderPass_Triangle>();
        if (!m_trianglePass->Initialize(m_device.get(), m_swapchain.get()))
        {
            spdlog::error("Failed to initialize RenderPass_Triangle");
            return false;
        }

        // TextureLoader初期化
        m_texLoader = std::make_unique<TextureLoader>();
        if (!m_texLoader->Init(m_device->GetDevice()))
        {
            spdlog::error("Failed to initialize TextureLoader");
            return false;
        }

        // TexturedQuad初期化
        m_texQuad = std::make_unique<RenderPass_TexturedQuad>();
        if (!m_texQuad->Initialize(m_device.get(), m_swapchain.get()))
        {
            spdlog::error("Failed to initialize RenderPass_TexturedQuad");
            return false;
        }

        // ImGui初期化
        if (!m_imgui.Init(m_device.get(), m_swapchain.get(), m_hwnd))
        {
            spdlog::error("Failed to initialize ImGui");
            return false;
        }

        m_running = true;
        spdlog::info("Application initialized successfully");
        return true;
    }

    int App::Run()
    {
        spdlog::info("Entering main loop...");
        MSG msg = {};
        while (m_running)
        {
            while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
            {
                if (msg.message == WM_QUIT)
                {
                    spdlog::info("Received WM_QUIT, exiting main loop");
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

        spdlog::info("Main loop ended");
        return static_cast<int>(msg.wParam);
    }

    LRESULT CALLBACK App::WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        if (ImGui_ImplWin32_WndProcHandler(hwnd, uMsg, wParam, lParam))
            return 1; // ImGui が処理した場合は早期リターン

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
                    {
                        UINT w = LOWORD(lParam);
                        UINT h = HIWORD(lParam);
                        if (w != 0 && h != 0 && app->m_swapchain) {
                            app->m_swapchain->Resize(w, h);
                        }
                        return 0;
                    }
                    case WM_DPICHANGED:
                    {
                        // DPIに応じてウィンドウ矩形が提案されるので、それを適用
                        RECT* const prcNewWindow = reinterpret_cast<RECT*>(lParam);
                        SetWindowPos(hwnd, nullptr,
                                     prcNewWindow->left, prcNewWindow->top,
                                     prcNewWindow->right - prcNewWindow->left,
                                     prcNewWindow->bottom - prcNewWindow->top,
                                     SWP_NOZORDER | SWP_NOACTIVATE);
                        return 0;
                    }
                    case WM_ENTERSIZEMOVE:
                        app->m_inSizeMove = true;
                        return 0;
                    case WM_EXITSIZEMOVE:
                        app->m_inSizeMove = false;
                        // 終了時に現在のクライアントサイズでResize（ドラッグ中の大量Resizeを避ける）
                        RECT rc; GetClientRect(hwnd, &rc);
                        if (app->m_swapchain) app->m_swapchain->Resize(rc.right - rc.left, rc.bottom - rc.top);
                        return 0;
            }
        }

        return DefWindowProcW(hwnd, uMsg, wParam, lParam);
    }

    void App::Update()
    {
        // 現在は何もしない
    }

    void App::Render()
    {
        // スワップチェーンが初期化されている場合のみレンダリング
        if (m_swapchain && m_swapchain->GetCurrentBackBuffer())
        {
            // 最小化チェック
            RECT rect;
            GetClientRect(m_hwnd, &rect);
            if (rect.right - rect.left == 0 || rect.bottom - rect.top == 0)
            {
                Sleep(1); // 負荷低減
                return;
            }

            // フレーム開始前に、前フレームからの遅延ロードを処理
            if (!m_pendingTexturePath.empty()) {
                auto path = m_pendingTexturePath; // 退避
                m_pendingTexturePath.clear();
                jisaku::TextureHandle h;
                m_device->UploadAndWait([&](ID3D12GraphicsCommandList* cmd) {
                    m_texQuad->GetTextureLoader()->LoadFromFile(m_device->GetDevice(), cmd, path, h, /*forceSRGB=*/true, /*generateMips=*/true);
                });
                m_texQuad->GetTextureLoader()->FlushUploads();
                if (h.resource) {
                    m_textures.push_back(h);
                    m_activeTex = (int)m_textures.size() - 1;
                    if (m_texQuad) m_texQuad->SetActiveSlot(h.slot);
                }
            }

            const float clear[4] = { 0.392f, 0.584f, 0.929f, 1.0f }; // CornflowerBlue-ish
            m_device->BeginFrame();
            m_imgui.NewFrame();
            
            // ImGuiデバッグウィンドウ
            if (ImGui::Begin("Debug")) {
                if (ImGui::Button("Add Texture...")) {
                    wchar_t path[MAX_PATH] = {};
                    OPENFILENAMEW ofn{};
                    ofn.lStructSize = sizeof(ofn);
                    ofn.hwndOwner = m_hwnd;
                    ofn.lpstrFilter = L"Images\0*.png;*.jpg;*.jpeg;*.bmp;*.tga;*.gif\0All\0*.*\0";
                    ofn.lpstrFile = path;
                    ofn.nMaxFile = MAX_PATH;
                    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
                    if (GetOpenFileNameW(&ofn)) {
                        // フレーム外でメインスレッドが安全なタイミングで実行するため、パスのみ保持
                        m_pendingTexturePath = path;
                    }
                }
                for (int i = 0; i < (int)m_textures.size(); ++i) {
                    char label[64]; sprintf_s(label, "Tex %d", i);
                    bool selected = (m_activeTex == i);
                    if (ImGui::Selectable(label, selected)) {
                        m_activeTex = i;
                        if (m_texQuad) m_texQuad->SetActiveSlot(m_textures[i].slot);
                    }
                }
                if (m_activeTex >= 0) {
                    ImGui::Text("Active: %d", m_activeTex);
                }
            }
            ImGui::End();
            
            m_renderPass->Execute(m_device->GetCommandList(), *m_swapchain, clear);
            m_texQuad->Execute(m_device->GetCommandList(), *m_swapchain);
            m_imgui.Render(m_device->GetCommandList());
            m_device->EndFrameAndPresent(*m_swapchain, true); // VSync有効
        }
    }
}
