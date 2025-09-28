#include "ImGuiLayer.h"
#include "DX12Device.h"
#include "Swapchain.h"
#include <imgui.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_dx12.h>
#include <spdlog/spdlog.h>
#include <chrono>

namespace jisaku
{
    ImGuiLayer::ImGuiLayer() : m_device(nullptr), m_swapchain(nullptr), m_hwnd(nullptr), m_srvDescriptorSize(0), m_vsyncEnabled(true), m_fps(0.0f), m_frameTime(0.0f)
    {
    }

    ImGuiLayer::~ImGuiLayer()
    {
        Shutdown();
    }

    bool ImGuiLayer::Initialize(DX12Device* device, Swapchain* swapchain, HWND hwnd)
    {
        m_device = device;
        m_swapchain = swapchain;
        m_hwnd = hwnd;

        if (!CreateDescriptorHeap())
        {
            spdlog::error("Failed to create descriptor heap for ImGui");
            return false;
        }

        // ImGuiコンテキスト作成
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

        // ImGuiスタイル設定
        SetupImGuiStyle();

        // Win32バックエンド初期化
        if (!ImGui_ImplWin32_Init(hwnd))
        {
            spdlog::error("Failed to initialize ImGui Win32 backend");
            return false;
        }

        // DX12バックエンド初期化
        if (!ImGui_ImplDX12_Init(device->GetDevice(), 2, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, m_srvHeap.Get(),
            m_srvHeap->GetCPUDescriptorHandleForHeapStart(),
            m_srvHeap->GetGPUDescriptorHandleForHeapStart()))
        {
            spdlog::error("Failed to initialize ImGui DX12 backend");
            return false;
        }

        spdlog::info("ImGuiLayer initialized successfully");
        return true;
    }

    void ImGuiLayer::Shutdown()
    {
        ImGui_ImplDX12_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();

        if (m_srvHeap)
        {
            m_srvHeap->Release();
            m_srvHeap.Reset();
        }
    }

    void ImGuiLayer::Update()
    {
        // FPS計算
        static auto lastTime = std::chrono::high_resolution_clock::now();
        auto currentTime = std::chrono::high_resolution_clock::now();
        auto deltaTime = std::chrono::duration<float>(currentTime - lastTime).count();
        lastTime = currentTime;

        m_frameTime = deltaTime;
        m_fps = 1.0f / deltaTime;

        // ImGuiフレーム開始
        ImGui_ImplDX12_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        // デバッグウィンドウ
        ImGui::Begin("Debug Info");
        ImGui::Text("FPS: %.1f", m_fps);
        ImGui::Text("Frame Time: %.3f ms", m_frameTime * 1000.0f);
        ImGui::Checkbox("VSync", &m_vsyncEnabled);
        ImGui::End();
    }

    void ImGuiLayer::Render()
    {
        // コマンドリストを取得
        ID3D12GraphicsCommandList* commandList = m_device->GetCommandList();

        // リソースバリア設定
        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barrier.Transition.pResource = m_swapchain->GetBackBuffer(m_swapchain->GetCurrentBackBufferIndex());
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

        commandList->ResourceBarrier(1, &barrier);

        // レンダーターゲットを設定
        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_swapchain->GetRTVHandle(m_swapchain->GetCurrentBackBufferIndex());
        commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

        // ImGuiレンダリング
        ImGui::Render();
        ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList);
    }

    bool ImGuiLayer::CreateDescriptorHeap()
    {
        D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
        srvHeapDesc.NumDescriptors = 2;
        srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

        HRESULT hr = m_device->GetDevice()->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&m_srvHeap));
        if (FAILED(hr))
        {
            spdlog::error("Failed to create SRV heap for ImGui: 0x{:x}", hr);
            return false;
        }

        m_srvDescriptorSize = m_device->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        return true;
    }

    void ImGuiLayer::SetupImGuiStyle()
    {
        ImGuiStyle& style = ImGui::GetStyle();
        style.WindowRounding = 5.0f;
        style.FrameRounding = 3.0f;
        style.GrabRounding = 3.0f;
        style.ScrollbarRounding = 3.0f;

        ImVec4* colors = style.Colors;
        colors[ImGuiCol_WindowBg] = ImVec4(0.06f, 0.06f, 0.06f, 0.94f);
        colors[ImGuiCol_Header] = ImVec4(0.20f, 0.20f, 0.20f, 0.54f);
        colors[ImGuiCol_HeaderHovered] = ImVec4(0.40f, 0.40f, 0.40f, 0.54f);
        colors[ImGuiCol_HeaderActive] = ImVec4(0.20f, 0.20f, 0.20f, 0.54f);
        colors[ImGuiCol_Button] = ImVec4(0.20f, 0.20f, 0.20f, 0.54f);
        colors[ImGuiCol_ButtonHovered] = ImVec4(0.40f, 0.40f, 0.40f, 0.54f);
        colors[ImGuiCol_ButtonActive] = ImVec4(0.20f, 0.20f, 0.20f, 0.54f);
    }
}
