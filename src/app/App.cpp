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
#include <chrono>

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

        // GPUタイマー初期化
        m_gpuTimer = std::make_unique<GPUTimer>();
        m_gpuTimer->Init(m_device.get(), 32);

        // 入力管理
        m_input = std::make_unique<InputManager>();
        
        // Raw Input API登録
        RAWINPUTDEVICE rid[1];
        rid[0].usUsagePage = 0x01; // Generic Desktop
        rid[0].usUsage = 0x02;     // Mouse
        rid[0].dwFlags = RIDEV_INPUTSINK; // フォーカスがなくても入力を受信
        rid[0].hwndTarget = m_hwnd;
        if (!RegisterRawInputDevices(rid, 1, sizeof(RAWINPUTDEVICE))) {
            spdlog::warn("Failed to register Raw Input device");
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
            case WM_KEYDOWN:
            case WM_SYSKEYDOWN:
                if (app->m_input) app->m_input->OnKeyEvent(wParam, true);
                // Escキーでゲーム終了
                if (wParam == VK_ESCAPE) {
                    PostQuitMessage(0);
                    return 0;
                }
                return 0;
            case WM_KEYUP:
            case WM_SYSKEYUP:
                if (app->m_input) app->m_input->OnKeyEvent(wParam, false);
                return 0;
            case WM_MOUSEMOVE:
            {
                static int lastX = (short)LOWORD(lParam), lastY = (short)HIWORD(lParam);
                int x = (short)LOWORD(lParam), y = (short)HIWORD(lParam);
                if (app->m_input) app->m_input->OnMouseMove(x - lastX, y - lastY);
                lastX = x; lastY = y;
                return 0;
            }
            case WM_LBUTTONDOWN: if (app->m_input) app->m_input->OnMouseButton(0, true); return 0;
            case WM_LBUTTONUP:   if (app->m_input) app->m_input->OnMouseButton(0, false); return 0;
            case WM_RBUTTONDOWN: if (app->m_input) app->m_input->OnMouseButton(1, true); return 0;
            case WM_RBUTTONUP:   if (app->m_input) app->m_input->OnMouseButton(1, false); return 0;
            case WM_MBUTTONDOWN: if (app->m_input) app->m_input->OnMouseButton(2, true); return 0;
            case WM_MBUTTONUP:   if (app->m_input) app->m_input->OnMouseButton(2, false); return 0;
            case WM_MOUSEWHEEL:  if (app->m_input) app->m_input->OnWheel((short)HIWORD(wParam)); return 0;
            case WM_INPUT:
                if (app->m_input) {
                    UINT dataSize = 0;
                    GetRawInputData((HRAWINPUT)lParam, RID_INPUT, nullptr, &dataSize, sizeof(RAWINPUTHEADER));
                    if (dataSize > 0) {
                        std::vector<BYTE> buffer(dataSize);
                        if (GetRawInputData((HRAWINPUT)lParam, RID_INPUT, buffer.data(), &dataSize, sizeof(RAWINPUTHEADER)) == dataSize) {
                            RAWINPUT* raw = (RAWINPUT*)buffer.data();
                            if (raw->header.dwType == RIM_TYPEMOUSE) {
                                app->m_input->OnRawMouseMove(raw->data.mouse.lLastX, raw->data.mouse.lLastY);
                            }
                        }
                    }
                }
                return 0;
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
            if (m_gpuTimer) m_gpuTimer->NewFrame();
            m_imgui.NewFrame();
            // 入力更新（デルタタイム計算）
            if (m_input) {
                // 簡易デルタタイム計算（将来は高精度タイマーへ）
                static auto lastTime = std::chrono::high_resolution_clock::now();
                auto currentTime = std::chrono::high_resolution_clock::now();
                float dt = std::chrono::duration<float>(currentTime - lastTime).count();
                lastTime = currentTime;
                
                // デルタタイムを適切な範囲にクランプ
                dt = (std::min)(dt, 1.0f/30.0f); // 最大30FPS相当
                
                m_input->UpdateCamera(dt);
                
                // 右クリック時はマウスカーソルを非表示にする（Raw Input使用）
                if (m_input->IsMouseDown(1)) { // 右クリック（ボタン1）
                    if (!m_cursorHidden) {
                        ShowCursor(FALSE); // カーソルを非表示
                        m_cursorHidden = true;
                        SetCapture(m_hwnd); // マウスキャプチャ（画面外でもマウスイベントを受信）
                    }
                } else {
                    if (m_cursorHidden) {
                        ShowCursor(TRUE);  // カーソルを表示
                        m_cursorHidden = false;
                        ReleaseCapture();  // マウスキャプチャ解除
                    }
                }
            }
            if (m_texQuad && m_input) {
                m_texQuad->SetCamera(m_input->GetCameraPosition(), m_input->GetCameraRotationQ());
            }
            
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

                // Mouse sensitivity control
                if (m_input) {
                    static float mouseSensitivity = m_input->GetMouseSensitivity();
                    if (ImGui::SliderFloat("Mouse Sensitivity", &mouseSensitivity, 0.001f, 0.02f)) {
                        m_input->SetMouseSensitivity(mouseSensitivity);
                    }
                }

                // Transform controls (tx, ty, rot, sx, sy)
                static float tx = 0.0f, ty = 0.0f, rot = 0.0f, sx = 256.0f, sy = 256.0f;
                ImGui::SliderFloat("Trans X", &tx, -500.0f, 500.0f);
                ImGui::SliderFloat("Trans Y", &ty, -500.0f, 500.0f);
                ImGui::SliderFloat("Rotate(deg)", &rot, -180.0f, 180.0f);
                ImGui::SliderFloat("Scale X", &sx, 0.1f, 5.0f);
                ImGui::SliderFloat("Scale Y", &sy, 0.1f, 5.0f);
                if (m_texQuad) m_texQuad->SetTransform(tx, ty, rot, sx, sy);
            }
            ImGui::End();
            if (m_gpuTimer) m_gpuTimer->DrawImGui();
            
            if (m_gpuTimer) m_gpuTimer->Begin(m_device->GetCommandList(), "Clear");
            m_renderPass->Execute(m_device->GetCommandList(), *m_swapchain, clear);
            if (m_gpuTimer) m_gpuTimer->End(m_device->GetCommandList(), "Clear");
            if (m_gpuTimer) m_gpuTimer->Begin(m_device->GetCommandList(), "TexturedQuad");
            m_texQuad->Execute(m_device->GetCommandList(), *m_swapchain);
            if (m_gpuTimer) m_gpuTimer->End(m_device->GetCommandList(), "TexturedQuad");

            if (m_gpuTimer) m_gpuTimer->Begin(m_device->GetCommandList(), "ImGui");
            m_imgui.Render(m_device->GetCommandList());
            if (m_gpuTimer) m_gpuTimer->End(m_device->GetCommandList(), "ImGui");

            if (m_gpuTimer) m_gpuTimer->Resolve(m_device->GetCommandList());
            m_device->EndFrameAndPresent(*m_swapchain, true); // VSync有効
            if (m_gpuTimer) m_gpuTimer->Collect();
            
            // 入力フレーム終了処理
            if (m_input) {
                m_input->NewFrame();
                // マウス中央固定フラグをリセット（右クリック状態をチェック）
                m_input->SetMouseCapture(m_input->IsMouseDown(1)); // 1 = 右クリック
            }
        }
    }
}
