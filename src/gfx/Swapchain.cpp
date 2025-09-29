#include "Swapchain.h"
#include "DX12Device.h"
#include <d3d12.h>
#include <dxgi1_6.h>
#include <spdlog/spdlog.h>
#include <fstream>

namespace jisaku
{
    Swapchain::Swapchain() : m_device(nullptr), m_rtvDescriptorSize(0), m_currentBackBufferIndex(0), m_width(0), m_height(0), m_rtvFormat(DXGI_FORMAT_R8G8B8A8_UNORM)
    {
    }

    Swapchain::~Swapchain()
    {
        Shutdown();
    }

    bool Swapchain::Initialize(DX12Device* device, HWND hwnd)
    {
        std::ofstream debugFile("swapchain_debug.txt");
        debugFile << "Swapchain::Initialize starting..." << std::endl;
        debugFile.flush();
        
        m_device = device;

        // ウィンドウサイズ取得
        debugFile << "Getting window size..." << std::endl;
        debugFile.flush();
        
        RECT rect;
        GetClientRect(hwnd, &rect);
        m_width = rect.right - rect.left;
        m_height = rect.bottom - rect.top;
        
        debugFile << "Window size: " << m_width << "x" << m_height << std::endl;
        debugFile.flush();
        
        // 最小サイズを確保
        if (m_width == 0) m_width = 1280;
        if (m_height == 0) m_height = 720;
        
        debugFile << "Adjusted window size: " << m_width << "x" << m_height << std::endl;
        debugFile.flush();

        debugFile << "Creating swapchain..." << std::endl;
        debugFile.flush();
        
        if (!CreateSwapchain(hwnd))
        {
            debugFile << "Failed to create swapchain" << std::endl;
            debugFile.flush();
            debugFile.close();
            spdlog::error("Failed to create swapchain");
            return false;
        }
        
        debugFile << "Swapchain created successfully" << std::endl;
        debugFile.flush();

        debugFile << "Creating RTV heap..." << std::endl;
        debugFile.flush();
        
        if (!CreateRTVHeap())
        {
            debugFile << "Failed to create RTV heap" << std::endl;
            debugFile.flush();
            debugFile.close();
            spdlog::error("Failed to create RTV heap");
            return false;
        }
        
        debugFile << "RTV heap created successfully" << std::endl;
        debugFile.flush();

        debugFile << "Creating RTVs..." << std::endl;
        debugFile.flush();
        
        if (!CreateRTVs())
        {
            debugFile << "Failed to create RTVs" << std::endl;
            debugFile.flush();
            debugFile.close();
            spdlog::error("Failed to create RTVs");
            return false;
        }
        
        debugFile << "RTVs created successfully" << std::endl;
        debugFile.flush();
        debugFile.close();

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

        if (m_swapChain)
        {
            m_swapChain->Release();
            m_swapChain.Reset();
        }
    }

    bool Swapchain::CreateSwapchain(HWND hwnd)
    {
        std::ofstream debugFile("swapchain_create_debug.txt");
        debugFile << "CreateSwapchain starting..." << std::endl;
        debugFile.flush();
        
        DXGI_SWAP_CHAIN_DESC1 swapchainDesc = {};
        swapchainDesc.BufferCount = 2;
        swapchainDesc.Width = m_width;
        swapchainDesc.Height = m_height;
        // 一部環境で _SRGB のスワップチェーン作成が INVALID_CALL となるため UNORM を使用
        m_rtvFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
        swapchainDesc.Format = m_rtvFormat;
        swapchainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapchainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        swapchainDesc.SampleDesc.Count = 1;
        swapchainDesc.Scaling = DXGI_SCALING_STRETCH;
        swapchainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;

        debugFile << "Calling GetFactory()..." << std::endl;
        debugFile.flush();
        
        auto factory = m_device->GetFactory();
        if (!factory)
        {
            debugFile << "Factory is null!" << std::endl;
            debugFile.flush();
            debugFile.close();
            return false;
        }
        
        debugFile << "Factory obtained successfully" << std::endl;
        debugFile.flush();
        
        debugFile << "Calling GetCommandQueue()..." << std::endl;
        debugFile.flush();
        
        auto commandQueue = m_device->GetCommandQueue();
        if (!commandQueue)
        {
            debugFile << "CommandQueue is null!" << std::endl;
            debugFile.flush();
            debugFile.close();
            return false;
        }
        
        debugFile << "CommandQueue obtained successfully" << std::endl;
        debugFile.flush();

        Microsoft::WRL::ComPtr<IDXGISwapChain1> swapchain;
        debugFile << "Creating swapchain..." << std::endl;
        debugFile.flush();
        
        HRESULT hr = factory->CreateSwapChainForHwnd(
            commandQueue,
            hwnd,
            &swapchainDesc,
            nullptr,
            nullptr,
            &swapchain
        );

        if (FAILED(hr))
        {
            debugFile << "Failed to create swapchain: 0x" << std::hex << hr << std::endl;
            debugFile.flush();
            debugFile.close();
            spdlog::error("Failed to create swapchain: 0x{:x}", hr);
            return false;
        }
        
        debugFile << "Swapchain created successfully" << std::endl;
        debugFile.flush();

        debugFile << "Querying swapchain3..." << std::endl;
        debugFile.flush();
        
        hr = swapchain.As(&m_swapChain);
        if (FAILED(hr))
        {
            debugFile << "Failed to query swapchain3: 0x" << std::hex << hr << std::endl;
            debugFile.flush();
            debugFile.close();
            spdlog::error("Failed to query swapchain3: 0x{:x}", hr);
            return false;
        }
        
        debugFile << "Swapchain3 queried successfully" << std::endl;
        debugFile.flush();
        debugFile.close();

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
            HRESULT hr = m_swapChain->GetBuffer(i, IID_PPV_ARGS(&m_backBuffers[i]));
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
        m_swapChain->Present(1, 0);
        m_currentBackBufferIndex = m_swapChain->GetCurrentBackBufferIndex();
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
        HRESULT hr = m_swapChain->ResizeBuffers(2, m_width, m_height, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, 0);
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

    UINT Swapchain::GetCurrentBackBufferIndex() const
    {
        return m_swapChain ? m_swapChain->GetCurrentBackBufferIndex() : 0;
    }

    ID3D12Resource* Swapchain::GetCurrentBackBuffer() const
    {
        return m_backBuffers[GetCurrentBackBufferIndex()].Get();
    }

    D3D12_CPU_DESCRIPTOR_HANDLE Swapchain::GetCurrentRTV() const
    {
        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
        rtvHandle.ptr += GetCurrentBackBufferIndex() * m_rtvDescriptorSize;
        return rtvHandle;
    }

    DXGI_FORMAT Swapchain::GetRTVFormat() const
    {
        return m_rtvFormat;
    }

    void Swapchain::Present(bool vsync)
    {
        m_swapChain->Present(vsync ? 1 : 0, 0);
    }
}
