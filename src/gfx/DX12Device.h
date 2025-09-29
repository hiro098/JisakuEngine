#pragma once

#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
#include <memory>
#include <functional>

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
        Microsoft::WRL::ComPtr<IDXGIFactory6> GetFactory() const { return m_factory; }
        
        // 新しいAPI
        ID3D12CommandQueue* GetQueue() const { return m_commandQueue.Get(); }
        ID3D12CommandAllocator* GetCmdAlloc(UINT /*frameIndex*/) const { return m_commandAllocator.Get(); }
        UINT GetFrameIndex() const { return m_frameIndex; }
        UINT GetFrameCount() const { return m_frameCount; }
        void WaitIdle();
        void BeginFrame();
        void EndFrameAndPresent(class Swapchain& swap, bool vsync);
        void ExecuteAndWait(std::function<void(ID3D12GraphicsCommandList*)> record);

        // アップロード専用（描画とは別の）コンテキスト
        void InitUploadContext(); // 初期化時に呼ぶ
        void UploadAndWait(std::function<void(ID3D12GraphicsCommandList*)> record);

    private:
        bool CreateDevice();
        bool CreateCommandQueue();
        bool CreateCommandAllocator();
        bool CreateCommandList();
        bool CreateFence();

        Microsoft::WRL::ComPtr<IDXGIFactory6> m_factory;
        Microsoft::WRL::ComPtr<ID3D12Device> m_device;
        Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_commandQueue;
        Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_commandAllocator;
        Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_commandList;
        Microsoft::WRL::ComPtr<ID3D12Fence> m_fence;
        UINT64 m_fenceValue;
        HANDLE m_fenceEvent;
        UINT m_frameIndex;
        UINT m_frameCount;

        // アップロード専用（描画とは別の）コンテキスト
        Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_uploadAlloc;
        Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_uploadCmd;
    };
}
