#include "DX12Device.h"
#include "Swapchain.h"
#include <d3d12.h>
#include <dxgi1_6.h>
#include <spdlog/spdlog.h>

namespace jisaku
{
    DX12Device::DX12Device() : m_fenceValue(0), m_fenceEvent(nullptr), m_frameIndex(0), m_frameCount(2)
    {
    }

    DX12Device::~DX12Device()
    {
        if (m_fenceEvent)
        {
            CloseHandle(m_fenceEvent);
        }
        Shutdown();
    }

    bool DX12Device::Initialize()
    {
        if (!CreateDevice())
        {
            spdlog::error("Failed to create D3D12 device");
            return false;
        }

        if (!CreateCommandQueue())
        {
            spdlog::error("Failed to create command queue");
            return false;
        }

        if (!CreateCommandAllocator())
        {
            spdlog::error("Failed to create command allocator");
            return false;
        }

        if (!CreateCommandList())
        {
            spdlog::error("Failed to create command list");
            return false;
        }

        if (!CreateFence())
        {
            spdlog::error("Failed to create fence");
            return false;
        }

        spdlog::info("DX12Device initialized successfully");
        return true;
    }

    void DX12Device::Shutdown()
    {
        if (m_commandList)
        {
            m_commandList->Release();
            m_commandList.Reset();
        }

        if (m_commandAllocator)
        {
            m_commandAllocator->Release();
            m_commandAllocator.Reset();
        }

        if (m_commandQueue)
        {
            m_commandQueue->Release();
            m_commandQueue.Reset();
        }

        if (m_device)
        {
            m_device->Release();
            m_device.Reset();
        }

        if (m_factory)
        {
            m_factory->Release();
            m_factory.Reset();
        }
    }

    bool DX12Device::CreateDevice()
    {
        // DXGI Factory作成
        HRESULT hr = CreateDXGIFactory2(0, IID_PPV_ARGS(&m_factory));
        if (FAILED(hr))
        {
            spdlog::error("Failed to create DXGI factory: 0x{:x}", hr);
            return false;
        }

        // アダプター検索
        Microsoft::WRL::ComPtr<IDXGIAdapter1> adapter;
        for (UINT adapterIndex = 0; DXGI_ERROR_NOT_FOUND != m_factory->EnumAdapters1(adapterIndex, &adapter); ++adapterIndex)
        {
            DXGI_ADAPTER_DESC1 desc;
            adapter->GetDesc1(&desc);

            if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
                continue;

            // D3D12デバイス作成
            hr = D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&m_device));
            if (SUCCEEDED(hr))
            {
                spdlog::info("D3D12 device created successfully");
                return true;
            }
        }

        spdlog::error("Failed to create D3D12 device");
        return false;
    }

    bool DX12Device::CreateCommandQueue()
    {
        D3D12_COMMAND_QUEUE_DESC queueDesc = {};
        queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

        HRESULT hr = m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_commandQueue));
        if (FAILED(hr))
        {
            spdlog::error("Failed to create command queue: 0x{:x}", hr);
            return false;
        }

        return true;
    }

    bool DX12Device::CreateCommandAllocator()
    {
        HRESULT hr = m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocator));
        if (FAILED(hr))
        {
            spdlog::error("Failed to create command allocator: 0x{:x}", hr);
            return false;
        }

        return true;
    }

    bool DX12Device::CreateCommandList()
    {
        HRESULT hr = m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocator.Get(), nullptr, IID_PPV_ARGS(&m_commandList));
        if (FAILED(hr))
        {
            spdlog::error("Failed to create command list: 0x{:x}", hr);
            return false;
        }

        // コマンドリストを閉じる（初期状態）
        m_commandList->Close();
        return true;
    }

    void DX12Device::BeginFrame()
    {
        m_commandAllocator->Reset();
        m_commandList->Reset(m_commandAllocator.Get(), nullptr);
    }

    void DX12Device::EndFrameAndPresent(Swapchain& swap, bool vsync)
    {
        m_commandList->Close();
        ID3D12CommandList* lists[] = { m_commandList.Get() };
        m_commandQueue->ExecuteCommandLists(1, lists);
        swap.Present(vsync);
        m_frameIndex = (m_frameIndex + 1) % 2; // ダブルバッファリング
    }

    bool DX12Device::CreateFence()
    {
        HRESULT hr = m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence));
        if (FAILED(hr))
        {
            spdlog::error("Failed to create fence: 0x{:x}", hr);
            return false;
        }

        m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        if (m_fenceEvent == nullptr)
        {
            spdlog::error("Failed to create fence event");
            return false;
        }

        return true;
    }

    void DX12Device::WaitIdle()
    {
        // m_queue, m_fence, m_fenceValue, m_fenceEvent を保持している前提
        const UINT64 signal = ++m_fenceValue;
        m_commandQueue->Signal(m_fence.Get(), signal);
        if (m_fence->GetCompletedValue() < signal) {
            m_fence->SetEventOnCompletion(signal, m_fenceEvent);
            WaitForSingleObject(m_fenceEvent, INFINITE);
        }
    }
}
