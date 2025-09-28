#include "Swapchain.h"
#include "DX12Device.h"
#include <d3d12.h>
#include <dxgi1_6.h>
#include <spdlog/spdlog.h>

namespace jisaku
{
    Swapchain::Swapchain() : m_device(nullptr), m_rtvDescriptorSize(0), m_currentBackBufferIndex(0), m_width(0), m_height(0)
    {
    }

    Swapchain::~Swapchain()
    {
        Shutdown();
    }

    bool Swapchain::Initialize(DX12Device* device, HWND hwnd)
    {
        m_device = device;

        // ウィンドウサイズ取得
        RECT rect;
        GetClientRect(hwnd, &rect);
        m_width = rect.right - rect.left;
        m_height = rect.bottom - rect.top;

        if (!CreateSwapchain(hwnd))
        {
            spdlog::error("Failed to create swapchain");
            return false;
        }

        if (!CreateRTVHeap())
        {
            spdlog::error("Failed to create RTV heap");
            return false;
        }

        if (!CreateRTVs())
        {
            spdlog::error("Failed to create RTVs");
            return false;
        }

        spdlog::info("Swapchain initialized successfully");
        return true;
    }

    void Swapchain::Shutdown()
    {
        for (auto& backBuffer : m_backBuffers)
        {
            if (backBuffer)
            {
                backBuffer->Release();
                backBuffer.Reset();
            }
        }

        if (m_rtvHeap)
        {
            m_rtvHeap->Release();
            m_rtvHeap.Reset();
        }

        if (m_swapchain)
        {
            m_swapchain->Release();
            m_swapchain.Reset();
        }
    }

    bool Swapchain::CreateSwapchain(HWND hwnd)
    {
        DXGI_SWAP_CHAIN_DESC1 swapchainDesc = {};
        swapchainDesc.BufferCount = 2;
        swapchainDesc.Width = m_width;
        swapchainDesc.Height = m_height;
        swapchainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
        swapchainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapchainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        swapchainDesc.SampleDesc.Count = 1;

        Microsoft::WRL::ComPtr<IDXGISwapChain1> swapchain;
        HRESULT hr = m_device->GetFactory()->CreateSwapChainForHwnd(
            m_device->GetCommandQueue(),
            hwnd,
            &swapchainDesc,
            nullptr,
            nullptr,
            &swapchain
        );

        if (FAILED(hr))
        {
            spdlog::error("Failed to create swapchain: 0x{:x}", hr);
            return false;
        }

        hr = swapchain.As(&m_swapchain);
        if (FAILED(hr))
        {
            spdlog::error("Failed to query swapchain3: 0x{:x}", hr);
            return false;
        }

        return true;
    }

    bool Swapchain::CreateRTVHeap()
    {
        D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
        rtvHeapDesc.NumDescriptors = 2;
        rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

        HRESULT hr = m_device->GetDevice()->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvHeap));
        if (FAILED(hr))
        {
            spdlog::error("Failed to create RTV heap: 0x{:x}", hr);
            return false;
        }

        m_rtvDescriptorSize = m_device->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        return true;
    }

    bool Swapchain::CreateRTVs()
    {
        for (UINT i = 0; i < 2; ++i)
        {
            HRESULT hr = m_swapchain->GetBuffer(i, IID_PPV_ARGS(&m_backBuffers[i]));
            if (FAILED(hr))
            {
                spdlog::error("Failed to get back buffer {}: 0x{:x}", i, hr);
                return false;
            }

            D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
            rtvHandle.ptr += i * m_rtvDescriptorSize;

            m_device->GetDevice()->CreateRenderTargetView(m_backBuffers[i].Get(), nullptr, rtvHandle);
        }

        return true;
    }

    void Swapchain::Present()
    {
        m_swapchain->Present(1, 0);
        m_currentBackBufferIndex = m_swapchain->GetCurrentBackBufferIndex();
    }

    void Swapchain::Resize(UINT width, UINT height)
    {
        if (m_width == width && m_height == height)
            return;

        m_width = width;
        m_height = height;

        // バックバッファをリリース
        for (auto& backBuffer : m_backBuffers)
        {
            if (backBuffer)
            {
                backBuffer->Release();
                backBuffer.Reset();
            }
        }

        // スワップチェーンをリサイズ
        HRESULT hr = m_swapchain->ResizeBuffers(2, m_width, m_height, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, 0);
        if (FAILED(hr))
        {
            spdlog::error("Failed to resize swapchain: 0x{:x}", hr);
            return;
        }

        // RTVを再作成
        CreateRTVs();
    }

    ID3D12Resource* Swapchain::GetBackBuffer(UINT index) const
    {
        return m_backBuffers[index].Get();
    }

    D3D12_CPU_DESCRIPTOR_HANDLE Swapchain::GetRTVHandle(UINT index) const
    {
        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
        rtvHandle.ptr += index * m_rtvDescriptorSize;
        return rtvHandle;
    }
}
