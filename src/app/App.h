#pragma once

#include <Windows.h>
#include <memory>
#include <wrl/client.h>
#include <atomic>
#include <thread>
#include "ui/ImGuiLayer.h"
#include "gfx/TextureLoader.h"
#include "gfx/GPUTimer.h"
#include "core/InputManager.h"
#include "gfx/ShaderReloader.h"

namespace jisaku
{
    class DX12Device;
    class Swapchain;
    class RenderPass_Clear;
    class RenderPass_Triangle;
    class RenderPass_TexturedQuad;
    class TextureLoader;
    struct TextureHandle;

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
        bool m_inSizeMove;

        std::unique_ptr<DX12Device> m_device;
        std::unique_ptr<Swapchain> m_swapchain;
        ImGuiLayer m_imgui;
        std::unique_ptr<RenderPass_Clear> m_renderPass;
        std::unique_ptr<RenderPass_Triangle> m_trianglePass;
        std::unique_ptr<RenderPass_TexturedQuad> m_texQuad;
        std::unique_ptr<TextureLoader> m_texLoader;
        TextureHandle m_loadedTex;

        // 遅延ロード用の一時パス
        std::wstring m_pendingTexturePath;

        // 非同期ロード制御
        std::atomic<bool> m_loading{ false };
        std::atomic<bool> m_loadedReady{ false };
        TextureHandle m_loadedTemp;
        std::thread m_loaderThread;

        // 複数テクスチャ管理
        std::vector<jisaku::TextureHandle> m_textures;
        int m_activeTex = -1;

        // GPUタイマー
        std::unique_ptr<jisaku::GPUTimer> m_gpuTimer;
        std::unique_ptr<jisaku::InputManager> m_input;
        
        // シェーダーホットリロード
        std::unique_ptr<jisaku::ShaderReloader> m_shaderReloader;
        double m_dtSmoothed = 0.0;
        
        // マウスカーソル状態管理
        bool m_cursorHidden = false;
    };
}
