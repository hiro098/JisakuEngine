#include "RenderPass_Triangle.h"
#include "DX12Device.h"
#include "Swapchain.h"
#include <d3d12.h>
#include <d3dcompiler.h>
#include <spdlog/spdlog.h>
#include <DirectXMath.h>

namespace jisaku
{
    struct Vertex
    {
        DirectX::XMFLOAT3 position;
        DirectX::XMFLOAT3 color;
    };

    RenderPass_Triangle::RenderPass_Triangle() : m_device(nullptr), m_swapchain(nullptr)
    {
    }

    RenderPass_Triangle::~RenderPass_Triangle()
    {
        Shutdown();
    }

    bool RenderPass_Triangle::Initialize(DX12Device* device, Swapchain* swapchain)
    {
        m_device = device;
        m_swapchain = swapchain;

        if (!CreateRootSignature())
        {
            spdlog::error("Failed to create root signature");
            return false;
        }

        if (!CreatePipelineState())
        {
            spdlog::error("Failed to create pipeline state");
            return false;
        }

        if (!CreateVertexBuffer())
        {
            spdlog::error("Failed to create vertex buffer");
            return false;
        }

        spdlog::info("RenderPass_Triangle initialized successfully");
        return true;
    }

    void RenderPass_Triangle::Shutdown()
    {
        if (m_vertexBuffer)
        {
            m_vertexBuffer->Release();
            m_vertexBuffer.Reset();
        }

        if (m_pipelineState)
        {
            m_pipelineState->Release();
            m_pipelineState.Reset();
        }

        if (m_rootSignature)
        {
            m_rootSignature->Release();
            m_rootSignature.Reset();
        }
    }

    bool RenderPass_Triangle::CreateRootSignature()
    {
        D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
        rootSignatureDesc.NumParameters = 0;
        rootSignatureDesc.pParameters = nullptr;
        rootSignatureDesc.NumStaticSamplers = 0;
        rootSignatureDesc.pStaticSamplers = nullptr;
        rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

        Microsoft::WRL::ComPtr<ID3DBlob> signature;
        Microsoft::WRL::ComPtr<ID3DBlob> error;
        HRESULT hr = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);
        if (FAILED(hr))
        {
            spdlog::error("Failed to serialize root signature: 0x{:x}", hr);
            return false;
        }

        hr = m_device->GetDevice()->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature));
        if (FAILED(hr))
        {
            spdlog::error("Failed to create root signature: 0x{:x}", hr);
            return false;
        }

        return true;
    }

    bool RenderPass_Triangle::CreatePipelineState()
    {
        // シェーダーコンパイル（簡易版）
        Microsoft::WRL::ComPtr<ID3DBlob> vertexShader;
        Microsoft::WRL::ComPtr<ID3DBlob> pixelShader;

        // 頂点シェーダー
        const char* vsCode = R"(
struct VSInput { float3 pos : POSITION; float3 col : COLOR; };
struct PSInput { float4 pos : SV_POSITION; float3 col : COLOR; };

PSInput VSMain(VSInput input) {
    PSInput o;
    o.pos = float4(input.pos, 1.0);
    o.col = input.col;
    return o;
}
)";

        const char* psCode = R"(
struct PSInput { float4 pos : SV_POSITION; float3 col : COLOR; };

float4 PSMain(PSInput input) : SV_TARGET {
    return float4(input.col, 1.0);
}
)";

        Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;
        
        HRESULT hr = D3DCompile(vsCode, strlen(vsCode), nullptr, nullptr, nullptr, "VSMain", "vs_5_0", 0, 0, &vertexShader, &errorBlob);
        if (FAILED(hr))
        {
            spdlog::error("Failed to compile vertex shader: 0x{:x}", hr);
            if (errorBlob)
            {
                spdlog::error("Shader error: {}", (char*)errorBlob->GetBufferPointer());
            }
            return false;
        }

        hr = D3DCompile(psCode, strlen(psCode), nullptr, nullptr, nullptr, "PSMain", "ps_5_0", 0, 0, &pixelShader, &errorBlob);
        if (FAILED(hr))
        {
            spdlog::error("Failed to compile pixel shader: 0x{:x}", hr);
            if (errorBlob)
            {
                spdlog::error("Shader error: {}", (char*)errorBlob->GetBufferPointer());
            }
            return false;
        }

        // 入力レイアウト
        D3D12_INPUT_ELEMENT_DESC inputElementDescs[] = {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
        };

        // パイプラインステート
        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
        psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
        psoDesc.pRootSignature = m_rootSignature.Get();
        psoDesc.VS = { vertexShader->GetBufferPointer(), vertexShader->GetBufferSize() };
        psoDesc.PS = { pixelShader->GetBufferPointer(), pixelShader->GetBufferSize() };
        
        // ラスタライザー設定
        D3D12_RASTERIZER_DESC rasterizerDesc = {};
        rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;
        rasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;
        rasterizerDesc.FrontCounterClockwise = FALSE;
        rasterizerDesc.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
        rasterizerDesc.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
        rasterizerDesc.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
        rasterizerDesc.DepthClipEnable = TRUE;
        rasterizerDesc.MultisampleEnable = FALSE;
        rasterizerDesc.AntialiasedLineEnable = FALSE;
        rasterizerDesc.ForcedSampleCount = 0;
        rasterizerDesc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
        psoDesc.RasterizerState = rasterizerDesc;
        
        // ブレンド設定
        D3D12_BLEND_DESC blendDesc = {};
        blendDesc.AlphaToCoverageEnable = FALSE;
        blendDesc.IndependentBlendEnable = FALSE;
        blendDesc.RenderTarget[0].BlendEnable = FALSE;
        blendDesc.RenderTarget[0].LogicOpEnable = FALSE;
        blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_ONE;
        blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_ZERO;
        blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
        blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
        blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
        blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
        blendDesc.RenderTarget[0].LogicOp = D3D12_LOGIC_OP_NOOP;
        blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
        psoDesc.BlendState = blendDesc;
        
        psoDesc.DepthStencilState.DepthEnable = FALSE;
        psoDesc.DepthStencilState.StencilEnable = FALSE;
        psoDesc.SampleMask = UINT_MAX;
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        psoDesc.NumRenderTargets = 1;
        psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
        psoDesc.SampleDesc.Count = 1;

        hr = m_device->GetDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState));
        if (FAILED(hr))
        {
            spdlog::error("Failed to create pipeline state: 0x{:x}", hr);
            return false;
        }

        return true;
    }

    bool RenderPass_Triangle::CreateVertexBuffer()
    {
        // 三角形の頂点データ
        Vertex triangleVertices[] = {
            { { 0.0f, 0.5f, 0.0f }, { 1.0f, 0.0f, 0.0f } },   // 上、赤
            { { 0.5f, -0.5f, 0.0f }, { 0.0f, 1.0f, 0.0f } },  // 右下、緑
            { { -0.5f, -0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f } }  // 左下、青
        };

        const UINT vertexBufferSize = sizeof(triangleVertices);

        D3D12_HEAP_PROPERTIES heapProps = {};
        heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
        heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

        D3D12_RESOURCE_DESC vertexBufferDesc = {};
        vertexBufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        vertexBufferDesc.Alignment = 0;
        vertexBufferDesc.Width = vertexBufferSize;
        vertexBufferDesc.Height = 1;
        vertexBufferDesc.DepthOrArraySize = 1;
        vertexBufferDesc.MipLevels = 1;
        vertexBufferDesc.Format = DXGI_FORMAT_UNKNOWN;
        vertexBufferDesc.SampleDesc.Count = 1;
        vertexBufferDesc.SampleDesc.Quality = 0;
        vertexBufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        vertexBufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

        HRESULT hr = m_device->GetDevice()->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &vertexBufferDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&m_vertexBuffer)
        );

        if (FAILED(hr))
        {
            spdlog::error("Failed to create vertex buffer: 0x{:x}", hr);
            return false;
        }

        // 頂点データをコピー
        UINT8* pVertexDataBegin;
        D3D12_RANGE readRange = { 0, 0 };
        hr = m_vertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin));
        if (FAILED(hr))
        {
            spdlog::error("Failed to map vertex buffer: 0x{:x}", hr);
            return false;
        }

        memcpy(pVertexDataBegin, triangleVertices, sizeof(triangleVertices));
        m_vertexBuffer->Unmap(0, nullptr);

        // 頂点バッファビュー
        m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
        m_vertexBufferView.StrideInBytes = sizeof(Vertex);
        m_vertexBufferView.SizeInBytes = vertexBufferSize;

        return true;
    }

    void RenderPass_Triangle::Execute(ID3D12GraphicsCommandList* cmd, Swapchain& swap)
    {
        // バックバッファをRTVにバリア
        auto back = swap.GetCurrentBackBuffer();
        D3D12_RESOURCE_BARRIER toRT = {};
        toRT.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        toRT.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        toRT.Transition.pResource = back;
        toRT.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
        toRT.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
        toRT.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        cmd->ResourceBarrier(1, &toRT);

        // RTV設定
        auto rtv = swap.GetCurrentRTV();
        cmd->OMSetRenderTargets(1, &rtv, FALSE, nullptr);

        // ビューポートとシザー設定
        D3D12_VIEWPORT vp{ 0.0f, 0.0f, (float)swap.GetWidth(), (float)swap.GetHeight(), 0.0f, 1.0f };
        D3D12_RECT sc{ 0, 0, (LONG)swap.GetWidth(), (LONG)swap.GetHeight() };
        cmd->RSSetViewports(1, &vp);
        cmd->RSSetScissorRects(1, &sc);

        // パイプライン設定
        cmd->SetPipelineState(m_pipelineState.Get());
        cmd->SetGraphicsRootSignature(m_rootSignature.Get());

        // 頂点バッファ設定
        cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        cmd->IASetVertexBuffers(0, 1, &m_vertexBufferView);

        // 描画
        cmd->DrawInstanced(3, 1, 0, 0);

        // バックバッファをPresentにバリア
        D3D12_RESOURCE_BARRIER toPresent = {};
        toPresent.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        toPresent.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        toPresent.Transition.pResource = back;
        toPresent.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
        toPresent.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
        toPresent.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        cmd->ResourceBarrier(1, &toPresent);
    }
}
