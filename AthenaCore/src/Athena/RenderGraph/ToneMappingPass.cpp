#include "Athena/RenderGraph/ToneMappingPass.h"
#include "Athena/Utils/Logger.h"
#include "Athena/Core/Device.h"
#include <d3dcompiler.h>
#include <stdexcept>

namespace Athena {

    ToneMappingPass::~ToneMappingPass() {
        // デストラクタ
    }

    void ToneMappingPass::Setup(PassSetupData& setupData) {
        Logger::Info("ToneMappingPass: Setup started");

        // パイプライン状態を作成
        CreateRootSignature(setupData.device);
        CreatePipelineState(setupData.device);

        // 定数バッファ初期化
        constantBuffer = std::make_unique<Buffer>();
        constantBuffer->Initialize(
            setupData.device,
            sizeof(ToneMappingConstants),
            BufferType::Constant,
            D3D12_HEAP_TYPE_UPLOAD
        );

        Logger::Info("ToneMappingPass: Setup completed");
    }

    void ToneMappingPass::Execute(const PassExecuteData& executeData) {
        auto* commandList = executeData.commandList;

        // 定数バッファ更新
        UpdateConstants();

        // パイプライン設定
        commandList->SetPipelineState(pipelineState.Get());
        commandList->SetGraphicsRootSignature(rootSignature.Get());

        // 定数バッファ設定
        commandList->SetGraphicsRootConstantBufferView(
            0, constantBuffer->GetGPUVirtualAddress()
        );

        // HDRテクスチャ設定
        if (hdrTexture && executeData.srvHeap) {
            ID3D12DescriptorHeap* heaps[] = { executeData.srvHeap };
            commandList->SetDescriptorHeaps(1, heaps);

            // HDRテクスチャのSRVを設定
            D3D12_GPU_DESCRIPTOR_HANDLE srvHandle = executeData.srvHeap->GetGPUDescriptorHandleForHeapStart();
            commandList->SetGraphicsRootDescriptorTable(1, srvHandle);
        }

        // フルスクリーンクアッドの描画
        commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        commandList->DrawInstanced(3, 1, 0, 0);
    }

    void ToneMappingPass::UpdateConstants() {
        ToneMappingConstants constants = {};
        constants.exposure = exposure;
        constants.gamma = gamma;
        constants.toneMappingMethod = static_cast<int>(toneMappingMethod);
        constants.whitePoint = whitePoint;
        constants.contrast = contrast;
        constants.brightness = brightness;
        constants.saturation = saturation;

        constantBuffer->Upload(&constants, sizeof(ToneMappingConstants));
    }

    void ToneMappingPass::CreateRootSignature(ID3D12Device* device) {
        // ルートパラメータ
        D3D12_ROOT_PARAMETER rootParams[2] = {};

        // 定数バッファ（b0）
        rootParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
        rootParams[0].Descriptor.ShaderRegister = 0;
        rootParams[0].Descriptor.RegisterSpace = 0;
        rootParams[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

        // HDRテクスチャ（t0）
        D3D12_DESCRIPTOR_RANGE texRange = {};
        texRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        texRange.NumDescriptors = 1;
        texRange.BaseShaderRegister = 0;
        texRange.RegisterSpace = 0;
        texRange.OffsetInDescriptorsFromTableStart = 0;

        rootParams[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        rootParams[1].DescriptorTable.NumDescriptorRanges = 1;
        rootParams[1].DescriptorTable.pDescriptorRanges = &texRange;
        rootParams[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

        // サンプラー（バイリニアフィルタリング）
        D3D12_STATIC_SAMPLER_DESC sampler = {};
        sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        sampler.MipLODBias = 0.0f;
        sampler.MaxAnisotropy = 1;
        sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
        sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;
        sampler.MinLOD = 0.0f;
        sampler.MaxLOD = D3D12_FLOAT32_MAX;
        sampler.ShaderRegister = 0;
        sampler.RegisterSpace = 0;
        sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

        // ルートシグネチャ記述
        D3D12_ROOT_SIGNATURE_DESC rootSigDesc = {};
        rootSigDesc.NumParameters = 2;
        rootSigDesc.pParameters = rootParams;
        rootSigDesc.NumStaticSamplers = 1;
        rootSigDesc.pStaticSamplers = &sampler;
        rootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;

        // シリアライズ
        ComPtr<ID3DBlob> signature;
        ComPtr<ID3DBlob> error;
        HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);
        if (FAILED(hr)) {
            if (error) {
                Logger::Error("Root signature error: %s", (char*)error->GetBufferPointer());
            }
            throw std::runtime_error("Failed to serialize root signature");
        }

        // ルートシグネチャ作成
        hr = device->CreateRootSignature(
            0,
            signature->GetBufferPointer(),
            signature->GetBufferSize(),
            IID_PPV_ARGS(&rootSignature)
        );
        if (FAILED(hr)) {
            throw std::runtime_error("Failed to create root signature");
        }

        Logger::Info("ToneMappingPass: Root signature created");
    }

    void ToneMappingPass::CreatePipelineState(ID3D12Device* device) {
        // シェーダーコンパイル
        auto vertexShader = CompileShader(L"../shaders/ToneMappingVS.hlsl", "main", "vs_5_1");
        auto pixelShader = CompileShader(L"../shaders/ToneMappingPS.hlsl", "main", "ps_5_1");

        // パイプライン状態記述
        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
        psoDesc.pRootSignature = rootSignature.Get();
        psoDesc.VS = { vertexShader->GetBufferPointer(), vertexShader->GetBufferSize() };
        psoDesc.PS = { pixelShader->GetBufferPointer(), pixelShader->GetBufferSize() };
        psoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
        psoDesc.SampleMask = UINT_MAX;
        psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
        psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
        psoDesc.RasterizerState.FrontCounterClockwise = FALSE;
        psoDesc.RasterizerState.DepthClipEnable = TRUE;
        psoDesc.DepthStencilState.DepthEnable = FALSE; // 深度テスト無効
        psoDesc.InputLayout = { nullptr, 0 }; // 頂点入力なし
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        psoDesc.NumRenderTargets = 1;
        psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM; // LDRフォーマット
        psoDesc.SampleDesc.Count = 1;

        // パイプライン状態作成
        HRESULT hr = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipelineState));
        if (FAILED(hr)) {
            throw std::runtime_error("Failed to create pipeline state");
        }

        Logger::Info("ToneMappingPass: Pipeline state created");
    }

    ComPtr<ID3DBlob> ToneMappingPass::CompileShader(const std::wstring& filepath,
                                                   const std::string& entryPoint,
                                                   const std::string& target) {
        UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;

        ComPtr<ID3DBlob> shader;
        ComPtr<ID3DBlob> error;

        HRESULT hr = D3DCompileFromFile(
            filepath.c_str(),
            nullptr, nullptr,
            entryPoint.c_str(),
            target.c_str(),
            compileFlags, 0,
            &shader, &error
        );

        if (FAILED(hr)) {
            if (error) {
                Logger::Error("Shader compilation failed: %s", (char*)error->GetBufferPointer());
            }
            Logger::Error("Failed to compile shader: %S", filepath.c_str());
            throw std::runtime_error("Failed to compile shader");
        }

        Logger::Info("Shader compiled: %S", filepath.c_str());
        return shader;
    }

} // namespace Athena