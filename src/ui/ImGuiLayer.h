#pragma once
#include <wrl.h>
#include <d3d12.h>
#include <dxgiformat.h>

namespace jisaku {
class DX12Device;
class Swapchain;

class ImGuiLayer {
public:
    ImGuiLayer() = default;
    ~ImGuiLayer();

    bool Init(DX12Device* dev, Swapchain* swap, HWND hwnd);
    void NewFrame();
    void Render(ID3D12GraphicsCommandList* cmdList);
    void Shutdown();

private:
    DX12Device* m_dev = nullptr;
    Swapchain*  m_swap = nullptr;

    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_srvHeap; // shader-visible for font
};
}