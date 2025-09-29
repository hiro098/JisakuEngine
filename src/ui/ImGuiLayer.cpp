#include "ui/ImGuiLayer.h"
#include "gfx/DX12Device.h"
#include "gfx/Swapchain.h"

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx12.h"

using Microsoft::WRL::ComPtr;

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace jisaku {

ImGuiLayer::~ImGuiLayer() { Shutdown(); }

bool ImGuiLayer::Init(DX12Device* dev, Swapchain* swap, HWND hwnd) {
    m_dev = dev; m_swap = swap;

    // ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

    // Style
    ImGui::StyleColorsDark();

    // SRV heap (shader visible, 1 descriptor for font)
    D3D12_DESCRIPTOR_HEAP_DESC desc = {};
    desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    desc.NumDescriptors = 1;
    desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    if (FAILED(m_dev->GetDevice()->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_srvHeap)))) return false;

    // Backends
    if (!ImGui_ImplWin32_Init(hwnd)) return false;

    DXGI_FORMAT fmt = m_swap->GetRTVFormat(); // e.g., DXGI_FORMAT_R8G8B8A8_UNORM / _SRGB
    ImGui_ImplDX12_Init(
        m_dev->GetDevice(),
        /*NumFramesInFlight*/  m_dev->GetFrameCount(),   // DX12Device に API が無ければ 2 or 3 を返す関数を追加
        fmt,
        m_srvHeap.Get(),
        m_srvHeap->GetCPUDescriptorHandleForHeapStart(),
        m_srvHeap->GetGPUDescriptorHandleForHeapStart()
    );

    return true;
}

void ImGuiLayer::NewFrame() {
    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    // --- Sample debug UI ---
    ImGui::Begin("Debug");
    ImGui::Text("JisakuEngine (DX12)");
    ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
    static bool vsync = true;
    ImGui::Checkbox("VSync", &vsync);
    // VSync のトグルは App 側で拾えるように後で接続しても良い
    ImGui::End();
}

void ImGuiLayer::Render(ID3D12GraphicsCommandList* cmd) {
    ImGui::Render();
    ID3D12DescriptorHeap* heaps[] = { m_srvHeap.Get() };
    cmd->SetDescriptorHeaps(1, heaps);
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), cmd);
}

void ImGuiLayer::Shutdown() {
    if (ImGui::GetCurrentContext()) {
        ImGui_ImplDX12_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
    }
    m_srvHeap.Reset();
}

} // namespace jisaku