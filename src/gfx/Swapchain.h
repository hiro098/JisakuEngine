#pragma once

#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
#include <Windows.h>

namespace jisaku
{
    class DX12Device;

    class Swapchain
    {
    public:
        Swapchain();
        ~Swapchain();

        bool Initialize(DX12Device* device, HWND hwnd);
        void Shutdown();

        void Present();
        bool Resize(UINT width, UINT height);

        ID3D12Resource* GetBackBuffer(UINT index) const;
        D3D12_CPU_DESCRIPTOR_HANDLE GetRTVHandle(UINT index) const;
        UINT GetCurrentBackBufferIndex() const;
        ID3D12Resource* GetCurrentBackBuffer() const;
        D3D12_CPU_DESCRIPTOR_HANDLE GetCurrentRTV() const;
        UINT GetWidth() const { return m_width; }
        UINT GetHeight() const { return m_height; }
        DXGI_FORMAT GetRTVFormat() const;
        void Present(bool vsync);

    private:
        bool CreateSwapchain(HWND hwnd);
        bool CreateRTVHeap();
        bool CreateRTVs();

        DX12Device* m_device;
        Microsoft::WRL::ComPtr<IDXGISwapChain3> m_swapChain;
        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
        Microsoft::WRL::ComPtr<ID3D12Resource> m_backBuffers[2];
        UINT m_rtvDescriptorSize;
        UINT m_currentBackBufferIndex;
        UINT m_width;
        UINT m_height;
        DXGI_FORMAT m_rtvFormat;
        UINT m_bufferCount;
    };
}
