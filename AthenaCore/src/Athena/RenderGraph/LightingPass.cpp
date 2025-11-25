#include "Athena/RenderGraph/LightingPass.h"
#include "Athena/Utils/Logger.h"
#include "Athena/Core/Device.h"
#include <d3dcompiler.h>
#include <stdexcept>
#include <algorithm>

namespace Athena {

    LightingPass::~LightingPass() {
        // デストラクタ
    }

    void LightingPass::Setup(PassSetupData& setupData) {
        Logger::Info("LightingPass: Setup started");

        // パイプライン状態を作成
        CreateRootSignature(setupData.device);
        CreatePipelineState(setupData.device);

        // 定数バッファ初期化
        constantBuffer = std::make_unique<Buffer>();
        constantBuffer->Initialize(
            setupData.device,
            sizeof(LightingConstants),
            BufferType::Constant,
            D3D12_HEAP_TYPE_UPLOAD
        );

        Logger::Info("LightingPass: Setup completed");
    }

    void LightingPass::Execute(const PassExecuteData& executeData) {
        auto* commandList = executeData.commandList;

        // 定数バッファ更新
        UpdateConstants();
        constantBuffer->Upload(&constants, sizeof(LightingConstants));

        // パイプライン設定
        commandList->SetPipelineState(pipelineState.Get());
        commandList->SetGraphicsRootSignature(rootSignature.Get());

        // 定数バッファ設定
        commandList->SetGraphicsRootConstantBufferView(
            0, constantBuffer->GetGPUVirtualAddress()
        );

        // G-Bufferテクスチャ設定
        if (executeData.srvHeap) {
            ID3D12DescriptorHeap* heaps[] = { executeData.srvHeap };
            commandList->SetDescriptorHeaps(1, heaps);

            // G-BufferのSRVを設定
            D3D12_GPU_DESCRIPTOR_HANDLE srvHandle = executeData.srvHeap->GetGPUDescriptorHandleForHeapStart();
            commandList->SetGraphicsRootDescriptorTable(1, srvHandle);
        }

        // フルスクリーン用のビューポートとシザー矩形を設定
        D3D12_VIEWPORT viewport = {};
        viewport.TopLeftX = 0.0f;
        viewport.TopLeftY = 0.0f;
        viewport.Width = static_cast<float>(renderTargetWidth);
        viewport.Height = static_cast<float>(renderTargetHeight);
        viewport.MinDepth = 0.0f;
        viewport.MaxDepth = 1.0f;
        commandList->RSSetViewports(1, &viewport);

        D3D12_RECT scissorRect = {};
        scissorRect.left = 0;
        scissorRect.top = 0;
        scissorRect.right = static_cast<LONG>(renderTargetWidth);
        scissorRect.bottom = static_cast<LONG>(renderTargetHeight);
        commandList->RSSetScissorRects(1, &scissorRect);

        // フルスクリーンクアッドの描画
        // 頂点バッファなしで三角形3つを描画してフルスクリーンをカバー
        commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        commandList->DrawInstanced(3, 1, 0, 0);
    }

    void LightingPass::AddDirectionalLight(const Vector3& direction, const Vector3& color, float intensity) {
        if (lightCount >= 8) {
            Logger::Warning("LightingPass: Maximum light count (8) reached");
            return;
        }

        LightData& light = lights[lightCount++];
        light.type = 0; // Directional light
        light.direction = direction;
        light.color = color;
        light.intensity = intensity;
        light.position = Vector3(0.0f, 0.0f, 0.0f); // Unused for directional light
        light.range = 0.0f; // Unused for directional light

        Logger::Info("LightingPass: Directional light added (total: %u)", lightCount);
    }

    void LightingPass::AddPointLight(const Vector3& position, const Vector3& color, float range, float intensity) {
        if (lightCount >= 8) {
            Logger::Warning("LightingPass: Maximum light count (8) reached");
            return;
        }

        LightData& light = lights[lightCount++];
        light.type = 1; // Point light
        light.position = position;
        light.color = color;
        light.intensity = intensity;
        light.range = range;
        light.direction = Vector3(0.0f, 0.0f, 0.0f); // Unused for point light

        Logger::Info("LightingPass: Point light added (total: %u)", lightCount);
    }

    void LightingPass::UpdateConstants() {
        constants.cameraPosition = cameraPosition;
        constants.exposure = exposure;
        constants.ambientColor = ambientColor;
        constants.lightCount = lightCount;
        constants.invViewMatrix = invViewMatrix.ToHLSL();  // HLSL用に転置
        constants.invProjMatrix = invProjMatrix.ToHLSL();  // HLSL用に転置

        // ライトデータをコピー
        for (uint32_t i = 0; i < lightCount && i < 8; ++i) {
            constants.lights[i] = lights[i];
        }
    }

    void LightingPass::CreateRootSignature(ID3D12Device* device) {
        // ルートパラメータ
        D3D12_ROOT_PARAMETER rootParams[2] = {};

        // 定数バッファ（b0）
        rootParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
        rootParams[0].Descriptor.ShaderRegister = 0;
        rootParams[0].Descriptor.RegisterSpace = 0;
        rootParams[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

        // G-Bufferテクスチャテーブル（t0, t1, t2）
        D3D12_DESCRIPTOR_RANGE gbufferRanges[3] = {};
        
        // アルベドテクスチャ（t0）
        gbufferRanges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        gbufferRanges[0].NumDescriptors = 1;
        gbufferRanges[0].BaseShaderRegister = 0;
        gbufferRanges[0].RegisterSpace = 0;
        gbufferRanges[0].OffsetInDescriptorsFromTableStart = 0;

        // 法線テクスチャ（t1）
        gbufferRanges[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        gbufferRanges[1].NumDescriptors = 1;
        gbufferRanges[1].BaseShaderRegister = 1;
        gbufferRanges[1].RegisterSpace = 0;
        gbufferRanges[1].OffsetInDescriptorsFromTableStart = 1;

        // デプステクスチャ（t2）
        gbufferRanges[2].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        gbufferRanges[2].NumDescriptors = 1;
        gbufferRanges[2].BaseShaderRegister = 2;
        gbufferRanges[2].RegisterSpace = 0;
        gbufferRanges[2].OffsetInDescriptorsFromTableStart = 2;

        rootParams[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        rootParams[1].DescriptorTable.NumDescriptorRanges = 3;
        rootParams[1].DescriptorTable.pDescriptorRanges = gbufferRanges;
        rootParams[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

        // サンプラー
        D3D12_STATIC_SAMPLER_DESC sampler = {};
        sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT; // G-Bufferは補間なし
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
        rootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE; // 入力アセンブラー不要

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

        Logger::Info("LightingPass: Root signature created");
    }

    void LightingPass::CreatePipelineState(ID3D12Device* device) {
        // シェーダーコンパイル
        auto vertexShader = CompileShader(L"../shaders/LightingVS.hlsl", "main", "vs_5_1");
        auto pixelShader = CompileShader(L"../shaders/LightingPS.hlsl", "main", "ps_5_1");

        // パイプライン状態記述（頂点入力なし）
        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
        psoDesc.pRootSignature = rootSignature.Get();
        psoDesc.VS = { vertexShader->GetBufferPointer(), vertexShader->GetBufferSize() };
        psoDesc.PS = { pixelShader->GetBufferPointer(), pixelShader->GetBufferSize() };
        psoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
        psoDesc.SampleMask = UINT_MAX;
        psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
        psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE; // フルスクリーンクアッド
        psoDesc.RasterizerState.FrontCounterClockwise = FALSE;
        psoDesc.RasterizerState.DepthClipEnable = TRUE;
        psoDesc.DepthStencilState.DepthEnable = FALSE; // 深度テスト無効
        psoDesc.InputLayout = { nullptr, 0 }; // 頂点入力なし
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        psoDesc.NumRenderTargets = 1;
        psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM; // 暫定的にLDRフォーマットに変更
        psoDesc.SampleDesc.Count = 1;

        // パイプライン状態作成
        HRESULT hr = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipelineState));
        if (FAILED(hr)) {
            throw std::runtime_error("Failed to create pipeline state");
        }

        Logger::Info("LightingPass: Pipeline state created");
    }

    ComPtr<ID3DBlob> LightingPass::CompileShader(const std::wstring& filepath,
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