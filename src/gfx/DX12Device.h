#pragma once

#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
#include <memory>

namespace jisaku
{
    class DX12Device
    {
    public:
        DX12Device();
        ~DX12Device();

        bool Initialize();
        void Shutdown();

        ID3D12Device* GetDevice() const { return m_device.Get(); }
        ID3D12CommandQueue* GetCommandQueue() const { return m_commandQueue.Get(); }
        ID3D12CommandAllocator* GetCommandAllocator() const { return m_commandAllocator.Get(); }
        ID3D12GraphicsCommandList* GetCommandList() const { return m_commandList.Get(); }

    private:
        bool CreateDevice();
        bool CreateCommandQueue();
        bool CreateCommandAllocator();
        bool CreateCommandList();

        Microsoft::WRL::ComPtr<IDXGIFactory4> m_factory;
        Microsoft::WRL::ComPtr<ID3D12Device> m_device;
        Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_commandQueue;
        Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_commandAllocator;
        Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_commandList;
    };
}
