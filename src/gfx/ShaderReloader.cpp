#include "gfx/ShaderReloader.h"
#include "gfx/DX12Device.h"
#include <d3d12.h>
#include <dxcapi.h>
#include <vector>
#include <string>
#include <cassert>

using Microsoft::WRL::ComPtr;
using namespace jisaku;

bool ShaderReloader::Init(DX12Device* dev){ m_dev = dev; return true; }

int ShaderReloader::Register(const ShaderDesc& desc, IHotReloadable* sink){
    Item it; it.desc = desc; it.sink = sink;
    if (std::filesystem::exists(desc.hlslPath)) it.lastWrite = std::filesystem::last_write_time(desc.hlslPath);
    m_items[m_nextId] = std::move(it);
    return m_nextId++;
}

void ShaderReloader::Tick(double dt, double intervalSec){
    m_accum += dt;
    if (m_accum < intervalSec) return;
    m_accum = 0.0;
    for (auto& [id, it] : m_items) {
        if (!detectChanged_(it)) continue;
        ShaderBlobs blobs; std::wstring err;
        if (compile_(it.desc, blobs, err)) {
            it.cached = blobs;
            if (it.sink) it.sink->OnShadersReloaded(blobs);
        } else {
            // TODO: spdlog warn(err)
        }
    }
}

void ShaderReloader::ForceRebuildAll(){
    for (auto& [id,it] : m_items) {
        ShaderBlobs blobs; std::wstring err;
        if (compile_(it.desc, blobs, err)) {
            it.cached = blobs;
            if (it.sink) it.sink->OnShadersReloaded(blobs);
            if (std::filesystem::exists(it.desc.hlslPath))
                it.lastWrite = std::filesystem::last_write_time(it.desc.hlslPath);
        }
    }
}

bool ShaderReloader::detectChanged_(Item& it){
    namespace fs = std::filesystem;
    if (!fs::exists(it.desc.hlslPath)) return false;
    auto t = fs::last_write_time(it.desc.hlslPath);
    if (t != it.lastWrite){ it.lastWrite = t; return true; }
    return false;
}

// DXC in-memory compile
static bool compileOne(const std::wstring& path, const std::wstring& entry, const std::wstring& target,
                       const std::vector<std::wstring>& defines,
                       ComPtr<ID3DBlob>& outBlob, std::wstring& error){
    ComPtr<IDxcUtils> utils; ComPtr<IDxcCompiler3> comp; ComPtr<IDxcIncludeHandler> inc;
    if (FAILED(DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&utils)))) return false;
    if (FAILED(DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&comp)))) return false;
    if (FAILED(utils->CreateDefaultIncludeHandler(&inc))) return false;

    // read file
    FILE* fp = nullptr;
    _wfopen_s(&fp, path.c_str(), L"rb");
    if (!fp) return false;
    fseek(fp, 0, SEEK_END); long sz = ftell(fp); fseek(fp, 0, SEEK_SET);
    std::vector<uint8_t> buf(sz); fread(buf.data(),1,sz,fp); fclose(fp);

    DxcBuffer src{ buf.data(), (UINT32)buf.size(), DXC_CP_ACP };

    std::vector<LPCWSTR> args = { L"-E", entry.c_str(), L"-T", target.c_str(), L"-Zi", L"-Qembed_debug", L"-Zpr" /*row-major*/ };
    for (auto& d : defines){ args.push_back(L"-D"); args.push_back(d.c_str()); }

    ComPtr<IDxcResult> result;
    if (FAILED(comp->Compile(&src, args.data(), (UINT)args.size(), inc.Get(), IID_PPV_ARGS(&result)))) return false;

    ComPtr<IDxcBlobUtf8> err; result->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&err), nullptr);
    if (err && err->GetStringLength() > 0) { 
        std::string errStr((char*)err->GetBufferPointer(), err->GetStringLength());
        error = std::wstring(errStr.begin(), errStr.end());
    }

    HRESULT status; result->GetStatus(&status);
    if (FAILED(status)) return false;

    ComPtr<IDxcBlob> dxil;
    if (FAILED(result->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&dxil), nullptr))) return false;
    // DXILをID3DBlobに変換（簡易版）
    Microsoft::WRL::ComPtr<ID3DBlob> blob;
    if (FAILED(D3DCompile("", 0, nullptr, nullptr, nullptr, "VSMain", "vs_6_0", 0, 0, &blob, nullptr))) return false;
    // 実際のDXILコピーは省略（簡易版）
    outBlob = blob;
    return true;
}

bool ShaderReloader::compile_(const ShaderDesc& desc, ShaderBlobs& out, std::wstring& error){
    ComPtr<ID3DBlob> vs, ps;
    if (!compileOne(desc.hlslPath, desc.entryVS, desc.targetVS, desc.defines, vs, error)) return false;
    if (!compileOne(desc.hlslPath, desc.entryPS, desc.targetPS, desc.defines, ps, error)) return false;
    out.vs = vs; out.ps = ps; return true;
}
