#include "RenderPass_TexturedQuad.h"
#include "DX12Device.h"
#include "Swapchain.h"
#include "TextureLoader.h"
#include <d3d12.h>
#include <d3dcompiler.h>
#include <spdlog/spdlog.h>
#include <DirectXMath.h>

namespace jisaku
{
    struct Vertex
    {
        DirectX::XMFLOAT3 pos;
        DirectX::XMFLOAT2 uv;
    };

    RenderPass_TexturedQuad::RenderPass_TexturedQuad() : m_device(nullptr)
    {
    }

    RenderPass_TexturedQuad::~RenderPass_TexturedQuad()
    {
    }

    bool RenderPass_TexturedQuad::Initialize(DX12Device* device, Swapchain* /*swapchain*/)
    {
        m_device = device;
        HRESULT hr;

        // ルートシグネチャ作成（SRVテーブル1個＋静的サンプラ）
        D3D12_DESCRIPTOR_RANGE srvRange = {};
        srvRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        srvRange.NumDescriptors = 1;
        srvRange.BaseShaderRegister = 0;
        srvRange.RegisterSpace = 0;
        srvRange.OffsetInDescriptorsFromTableStart = 0;

        D3D12_ROOT_PARAMETER rootParams[1] = {};
        rootParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        rootParams[0].DescriptorTable.NumDescriptorRanges = 1;
        rootParams[0].DescriptorTable.pDescriptorRanges = &srvRange;
        rootParams[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

        D3D12_STATIC_SAMPLER_DESC staticSampler = {};
        staticSampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        staticSampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        staticSampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        staticSampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        staticSampler.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
        staticSampler.ShaderRegister = 0;
        staticSampler.RegisterSpace = 0;
        staticSampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

        D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
        rootSignatureDesc.NumParameters = 1;
        rootSignatureDesc.pParameters = rootParams;
        rootSignatureDesc.NumStaticSamplers = 1;
        rootSignatureDesc.pStaticSamplers = &staticSampler;
        rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

        Microsoft::WRL::ComPtr<ID3DBlob> signature;
        Microsoft::WRL::ComPtr<ID3DBlob> error;
        hr = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);
        if (FAILED(hr))
        {
            if (error)
            {
                spdlog::error("Failed to serialize root signature: {}", (char*)error->GetBufferPointer());
            }
            return false;
        }

        hr = m_device->GetDevice()->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature));
        if (FAILED(hr))
        {
            spdlog::error("Failed to create root signature: 0x{:x}", hr);
            return false;
        }

        // シェーダーコンパイル
        Microsoft::WRL::ComPtr<ID3DBlob> vertexShader;
        Microsoft::WRL::ComPtr<ID3DBlob> pixelShader;

        hr = D3DCompileFromFile(L"shaders/TexturedQuad.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "VSMain", "vs_5_0", 0, 0, &vertexShader, &error);
        if (FAILED(hr))
        {
            if (error)
            {
                spdlog::error("Failed to compile VS: {}", (char*)error->GetBufferPointer());
            }
            return false;
        }

        hr = D3DCompileFromFile(L"shaders/TexturedQuad.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "PSMain", "ps_5_0", 0, 0, &pixelShader, &error);
        if (FAILED(hr))
        {
            if (error)
            {
                spdlog::error("Failed to compile PS: {}", (char*)error->GetBufferPointer());
            }
            return false;
        }

        // 入力レイアウト
        D3D12_INPUT_ELEMENT_DESC inputElementDescs[] = {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
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
        rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE; // 両面描画（向き依存を排除）
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

        // 頂点バッファ作成（四角形）
        Vertex quadVertices[] = {
            { { -0.5f, -0.5f, 0.0f }, { 0.0f, 1.0f } }, // 左下
            { {  0.5f, -0.5f, 0.0f }, { 1.0f, 1.0f } }, // 右下
            { {  0.5f,  0.5f, 0.0f }, { 1.0f, 0.0f } }, // 右上
            { { -0.5f,  0.5f, 0.0f }, { 0.0f, 0.0f } }  // 左上
        };

        const UINT vertexBufferSize = sizeof(quadVertices);

        D3D12_HEAP_PROPERTIES heapProps = {};
        heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
        heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        heapProps.CreationNodeMask = 1;
        heapProps.VisibleNodeMask = 1;

        D3D12_RESOURCE_DESC resourceDesc = {};
        resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        resourceDesc.Alignment = 0;
        resourceDesc.Width = vertexBufferSize;
        resourceDesc.Height = 1;
        resourceDesc.DepthOrArraySize = 1;
        resourceDesc.MipLevels = 1;
        resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
        resourceDesc.SampleDesc.Count = 1;
        resourceDesc.SampleDesc.Quality = 0;
        resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

        hr = m_device->GetDevice()->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &resourceDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&m_vertexBuffer)
        );
        if (FAILED(hr))
        {
            spdlog::error("Failed to create vertex buffer: 0x{:x}", hr);
            return false;
        }

        UINT8* pVertexDataBegin;
        D3D12_RANGE readRange = { 0, 0 };
        hr = m_vertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin));
        if (FAILED(hr))
        {
            spdlog::error("Failed to map vertex buffer: 0x{:x}", hr);
            return false;
        }
        memcpy(pVertexDataBegin, quadVertices, vertexBufferSize);
        m_vertexBuffer->Unmap(0, nullptr);

        m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
        m_vertexBufferView.StrideInBytes = sizeof(Vertex);
        m_vertexBufferView.SizeInBytes = vertexBufferSize;

        // インデックスバッファ作成
        uint16_t indices[] = {
            0, 1, 2,  // 最初の三角形 (時計回り)
            0, 2, 3   // 2番目の三角形 (時計回り)
        };

        const UINT indexBufferSize = sizeof(indices);

        resourceDesc.Width = indexBufferSize;
        hr = m_device->GetDevice()->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &resourceDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&m_indexBuffer)
        );
        if (FAILED(hr))
        {
            spdlog::error("Failed to create index buffer: 0x{:x}", hr);
            return false;
        }

        UINT8* pIndexDataBegin;
        hr = m_indexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pIndexDataBegin));
        if (FAILED(hr))
        {
            spdlog::error("Failed to map index buffer: 0x{:x}", hr);
            return false;
        }
        memcpy(pIndexDataBegin, indices, indexBufferSize);
        m_indexBuffer->Unmap(0, nullptr);

        m_indexBufferView.BufferLocation = m_indexBuffer->GetGPUVirtualAddress();
        m_indexBufferView.Format = DXGI_FORMAT_R16_UINT;
        m_indexBufferView.SizeInBytes = indexBufferSize;

        // TextureLoader初期化
        m_textureLoader = std::make_unique<TextureLoader>();
        if (!m_textureLoader->Init(m_device->GetDevice()))
        {
            spdlog::error("Failed to initialize TextureLoader");
            return false;
        }

        // テクスチャアップロード用に専用のコマンドアロケータ/コマンドリストを作成
        Microsoft::WRL::ComPtr<ID3D12CommandAllocator> uploadAllocator;
        HRESULT uploadHr = m_device->GetDevice()->CreateCommandAllocator(
            D3D12_COMMAND_LIST_TYPE_DIRECT,
            IID_PPV_ARGS(&uploadAllocator));
        if (FAILED(uploadHr))
        {
            spdlog::error("Failed to create upload command allocator: 0x{:x}", uploadHr);
            return false;
        }

        Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> uploadCmd;
        uploadHr = m_device->GetDevice()->CreateCommandList(
            0,
            D3D12_COMMAND_LIST_TYPE_DIRECT,
            uploadAllocator.Get(),
            nullptr,
            IID_PPV_ARGS(&uploadCmd));
        if (FAILED(uploadHr))
        {
            spdlog::error("Failed to create upload command list: 0x{:x}", uploadHr);
            return false;
        }

        // チェッカーテクスチャ作成
        m_texture = m_textureLoader->CreateCheckerboard(m_device->GetDevice(), uploadCmd.Get(), 256, 32);
        if (!m_texture.resource)
        {
            spdlog::error("Failed to create checkerboard texture");
            return false;
        }

        // コマンドリストを閉じて実行
        uploadCmd->Close();
        ID3D12CommandList* lists[] = { uploadCmd.Get() };
        m_device->GetCommandQueue()->ExecuteCommandLists(1, lists);
        m_device->WaitIdle();
        spdlog::info("Texture upload commands executed and GPU is idle");

        // アップロードリソースを解放（WaitIdleの後）
        m_textureLoader->FlushUploads();

        spdlog::info("RenderPass_TexturedQuad initialized successfully");
        return true;
    }

    void RenderPass_TexturedQuad::Execute(ID3D12GraphicsCommandList* cmd, Swapchain& swap)
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

        // 描画直前の固定順序セット
        // ①自前ヒープだけをセット（imgui等が上書きする可能性があるため）
        ID3D12DescriptorHeap* heaps[] = { m_textureLoader->GetSrvHeap() };
        cmd->SetDescriptorHeaps(1, heaps);

        // ②ルート→PSO→IA→VP/Scissor
        cmd->SetGraphicsRootSignature(m_rootSignature.Get());
        cmd->SetPipelineState(m_pipelineState.Get());

        // ③SRV テーブルを ルートパラメータ[0] に渡す
        D3D12_GPU_DESCRIPTOR_HANDLE gpuSrv = m_texture.srvGPU;
        if (m_activeSlot != UINT32_MAX && m_textureLoader->IsValidSlot(m_activeSlot)) {
            gpuSrv = { m_textureLoader->GetSrvHeap()->GetGPUDescriptorHandleForHeapStart().ptr +
                       UINT64(m_activeSlot) * UINT64(m_textureLoader->GetDescriptorSize()) };
        }
        cmd->SetGraphicsRootDescriptorTable(0, gpuSrv);
        spdlog::info("SetGraphicsRootDescriptorTable(0, {})", gpuSrv.ptr);

        // 頂点バッファ設定
        cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        cmd->IASetVertexBuffers(0, 1, &m_vertexBufferView);
        cmd->IASetIndexBuffer(&m_indexBufferView);
        cmd->DrawIndexedInstanced(6, 1, 0, 0, 0);

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

    void RenderPass_TexturedQuad::SetTexture(const TextureHandle& h)
    {
        m_texture = h;
        m_activeSlot = h.slot;
    }

    void RenderPass_TexturedQuad::SetActiveSlot(uint32_t slot)
    {
        m_activeSlot = slot;
    }
}
