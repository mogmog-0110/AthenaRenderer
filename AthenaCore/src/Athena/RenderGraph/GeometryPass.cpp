#include "Athena/RenderGraph/GeometryPass.h"
#include "Athena/Utils/Logger.h"
#include "Athena/Core/Device.h"
#include <d3dcompiler.h>
#include <stdexcept>

namespace Athena {

    GeometryPass::~GeometryPass() {
    }

    void GeometryPass::Setup(PassSetupData& setupData) {
        Logger::Info("GeometryPass: Setup started");

        // デバイス参照を保存
        device = setupData.device;
        
        // ルートシグネチャを作成（共通）
        CreateRootSignature(setupData.device);
        
        // パイプライン状態を作成（デフォルトはフォワード）
        CreateForwardPipelineState(setupData.device);

        // バッファ初期化
        constantBuffer = std::make_unique<Buffer>();
        constantBuffer->Initialize(
            setupData.device,
            sizeof(GeometryConstants),
            BufferType::Constant,
            D3D12_HEAP_TYPE_UPLOAD
        );

        // 既にキャッシュされた頂点データがあれば、バッファを作成
        if (!cachedVertices.empty() && !cachedIndices.empty()) {
            Logger::Info("GeometryPass: Creating buffers from cached data");
            UpdateBuffers(setupData.device);
        } else {
            Logger::Warning("GeometryPass: No cached vertex data available during Setup");
        }

        Logger::Info("GeometryPass: Setup completed");
    }

    void GeometryPass::Execute(const PassExecuteData& executeData) {
        auto* commandList = executeData.commandList;

        // レンダリングモードに応じてパイプライン状態を更新
        static RenderMode lastMode = RenderMode::Forward;  // 初期値
        if (renderMode != lastMode) {
            if (renderMode == RenderMode::Forward) {
                CreateForwardPipelineState(device);
                Logger::Info("GeometryPass: Switched to Forward rendering mode");
            } else {
                CreateDeferredPipelineState(device);
                Logger::Info("GeometryPass: Switched to Deferred rendering mode");
            }
            lastMode = renderMode;
        }

        // バッファを更新（必要に応じて）
        if (buffersDirty && device) {
            Logger::Info("GeometryPass: Updating buffers during Execute");
            UpdateBuffers(device);
        }

        // 定数バッファ更新
        constants.mvpMatrix = worldMatrix * viewMatrix * projMatrix;
        constants.mvpMatrix = constants.mvpMatrix.Transpose();
        constants.worldMatrix = worldMatrix.Transpose();
        constants.viewMatrix = viewMatrix.Transpose();
        constants.projMatrix = projMatrix.Transpose();
        constants.cameraPosition = cameraPosition;
        constants.lightDirection = lightDirection;
        constants.lightColor = lightColor;

        constantBuffer->Upload(&constants, sizeof(GeometryConstants));

        // レンダーターゲット設定（モード別）
        if (renderMode == RenderMode::Deferred && gbufferRTVHandles[0].ptr != 0) {
            // G-Buffer用レンダーターゲット設定（MRT）
            // ハンドルは外部から設定済み
            
            // レンダーターゲットをクリア（リソース作成時の最適化クリア値と一致させる）
            float clearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
            for (int i = 0; i < 3; ++i) {
                if (gbufferRTVHandles[i].ptr != 0) {
                    commandList->ClearRenderTargetView(gbufferRTVHandles[i], clearColor, 0, nullptr);
                }
            }
            if (gbufferDSVHandle.ptr != 0) {
                commandList->ClearDepthStencilView(gbufferDSVHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
            }
            
            // MRTを設定
            D3D12_CPU_DESCRIPTOR_HANDLE* dsvPtr = (gbufferDSVHandle.ptr != 0) ? &gbufferDSVHandle : nullptr;
            commandList->OMSetRenderTargets(3, gbufferRTVHandles, FALSE, dsvPtr);
            
            // ビューポートとシザー矩形を設定（G-Bufferのサイズに合わせる）
            if (gbufferAlbedo) {
                D3D12_VIEWPORT viewport = {};
                viewport.TopLeftX = 0.0f;
                viewport.TopLeftY = 0.0f;
                viewport.Width = static_cast<float>(gbufferAlbedo->GetWidth());
                viewport.Height = static_cast<float>(gbufferAlbedo->GetHeight());
                viewport.MinDepth = 0.0f;
                viewport.MaxDepth = 1.0f;
                commandList->RSSetViewports(1, &viewport);
                
                D3D12_RECT scissorRect = {};
                scissorRect.left = 0;
                scissorRect.top = 0;
                scissorRect.right = static_cast<LONG>(gbufferAlbedo->GetWidth());
                scissorRect.bottom = static_cast<LONG>(gbufferAlbedo->GetHeight());
                commandList->RSSetScissorRects(1, &scissorRect);
            }
            
            Logger::Info("GeometryPass: G-Buffer render targets configured (MRT)");
        }

        // パイプライン設定
        commandList->SetPipelineState(pipelineState.Get());
        commandList->SetGraphicsRootSignature(rootSignature.Get());

        // 定数バッファ設定
        commandList->SetGraphicsRootConstantBufferView(
            0, constantBuffer->GetGPUVirtualAddress()
        );

        // テクスチャ設定（メインレンダリングとの統合）
        if (executeData.srvHeap) {
            // メインレンダリングのSRVデスクリプタヒープを使用
            ID3D12DescriptorHeap* heaps[] = { executeData.srvHeap };
            commandList->SetDescriptorHeaps(1, heaps);
            
            // メインテクスチャのSRVを設定（スロット1）
            // メインレンダリングのテクスチャSRVは通常インデックス1にある
            D3D12_GPU_DESCRIPTOR_HANDLE srvHandle = executeData.srvHeap->GetGPUDescriptorHandleForHeapStart();
            if (device) {
                // メインレンダリングのテクスチャSRVはCBVの後（インデックス1）
                srvHandle.ptr += device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
            }
            commandList->SetGraphicsRootDescriptorTable(1, srvHandle);
            
            Logger::Info("GeometryPass: Using shared texture SRV, ObjectID: %u", constants.objectID);
        } else {
            // テクスチャなしで描画（単色）
            Logger::Info("GeometryPass: Rendering without texture, ObjectID: %u", constants.objectID);
        }

        // 頂点バッファとインデックスバッファ設定
        if (vertexBuffer && indexBuffer && indexCount > 0) {
            D3D12_VERTEX_BUFFER_VIEW vbv = vertexBuffer->GetVertexBufferView();
            vbv.StrideInBytes = sizeof(Vertex);

            D3D12_INDEX_BUFFER_VIEW ibv = indexBuffer->GetIndexBufferView();

            commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            commandList->IASetVertexBuffers(0, 1, &vbv);
            commandList->IASetIndexBuffer(&ibv);

            // 描画
            commandList->DrawIndexedInstanced(indexCount, 1, 0, 0, 0);
            const char* modeStr = (renderMode == RenderMode::Forward) ? "Forward" : "Deferred";
            Logger::Info("GeometryPass::Execute: Drew %u indices in %s mode", indexCount, modeStr);
        } else {
            Logger::Warning("GeometryPass::Execute: No valid vertex/index buffers to render (vb:%s, ib:%s, count:%u)", 
                          vertexBuffer ? "OK" : "NULL", 
                          indexBuffer ? "OK" : "NULL", 
                          indexCount);
        }
    }

    void GeometryPass::SetVertexData(const Vertex* vertices, uint32_t vertexCount,
                                    const uint32_t* indices, uint32_t indexCount) {
        this->vertexCount = vertexCount;
        this->indexCount = indexCount;

        // 頂点データをキャッシュ
        cachedVertices.clear();
        cachedVertices.resize(vertexCount);
        for (uint32_t i = 0; i < vertexCount; ++i) {
            cachedVertices[i] = vertices[i];
        }

        // インデックスデータをキャッシュ
        cachedIndices.clear();
        cachedIndices.resize(indexCount);
        for (uint32_t i = 0; i < indexCount; ++i) {
            cachedIndices[i] = indices[i];
        }

        buffersDirty = true;
        Logger::Info("GeometryPass::SetVertexData: Vertex data cached (vertices: %u, indices: %u)", vertexCount, indexCount);
    }

    void GeometryPass::SetTransform(const Matrix4x4& world, const Matrix4x4& view, const Matrix4x4& proj) {
        worldMatrix = world;
        viewMatrix = view;
        projMatrix = proj;
    }

    void GeometryPass::CreateRootSignature(ID3D12Device* device) {
        // 従来方式ルートシグネチャ
        D3D12_ROOT_PARAMETER rootParams[2] = {};

        // 定数バッファ（b0）
        rootParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
        rootParams[0].Descriptor.ShaderRegister = 0;
        rootParams[0].Descriptor.RegisterSpace = 0;
        rootParams[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

        // テクスチャテーブル（t0）
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

        // サンプラー
        D3D12_STATIC_SAMPLER_DESC sampler = {};
        sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        sampler.MipLODBias = 0.0f;
        sampler.MaxAnisotropy = 1;
        sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
        sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
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
        rootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

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

        Logger::Info("GeometryPass: Root signature created");
    }

    void GeometryPass::CreatePipelineState(ID3D12Device* device) {
        if (renderMode == RenderMode::Forward) {
            CreateForwardPipelineState(device);
        } else {
            CreateDeferredPipelineState(device);
        }
    }

    void GeometryPass::CreateForwardPipelineState(ID3D12Device* device) {
        auto vertexShader = CompileShader(L"../shaders/GeometryVS.hlsl", "main", "vs_5_1");
        auto pixelShader = CompileShader(L"../shaders/GeometryPS.hlsl", "main", "ps_5_1");

        D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
            {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
            {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
            {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        };

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
        psoDesc.DepthStencilState.DepthEnable = TRUE;
        psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
        psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
        psoDesc.InputLayout = { inputLayout, _countof(inputLayout) };
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        psoDesc.NumRenderTargets = 1;
        psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
        psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
        psoDesc.SampleDesc.Count = 1;

        HRESULT hr = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipelineState));
        if (FAILED(hr)) {
            throw std::runtime_error("Failed to create forward pipeline state");
        }

        Logger::Info("GeometryPass: Forward pipeline state created");
    }

    void GeometryPass::CreateDeferredPipelineState(ID3D12Device* device) {
        auto vertexShader = CompileShader(L"../shaders/GeometryVS.hlsl", "main", "vs_5_1");
        auto pixelShader = CompileShader(L"../shaders/GeometryDeferredPS.hlsl", "main", "ps_5_1");

        D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
            {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
            {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
            {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        };

        // G-Buffer用のブレンド設定（3つのレンダーターゲット）
        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
        psoDesc.pRootSignature = rootSignature.Get();
        psoDesc.VS = { vertexShader->GetBufferPointer(), vertexShader->GetBufferSize() };
        psoDesc.PS = { pixelShader->GetBufferPointer(), pixelShader->GetBufferSize() };
        
        // 3つのレンダーターゲット用のブレンド設定
        for (int i = 0; i < 3; ++i) {
            psoDesc.BlendState.RenderTarget[i].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
        }
        
        psoDesc.SampleMask = UINT_MAX;
        psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
        psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
        psoDesc.RasterizerState.FrontCounterClockwise = FALSE;
        psoDesc.RasterizerState.DepthClipEnable = TRUE;
        psoDesc.DepthStencilState.DepthEnable = TRUE;
        psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
        psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
        psoDesc.InputLayout = { inputLayout, _countof(inputLayout) };
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        
        // Multiple Render Targets (MRT) for G-Buffer
        psoDesc.NumRenderTargets = 3;
        psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;  // Albedo + Metallic
        psoDesc.RTVFormats[1] = DXGI_FORMAT_R8G8B8A8_UNORM;  // Normal + Roughness
        psoDesc.RTVFormats[2] = DXGI_FORMAT_R8G8B8A8_UNORM;  // Material + ObjectID
        psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
        psoDesc.SampleDesc.Count = 1;

        HRESULT hr = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipelineState));
        if (FAILED(hr)) {
            throw std::runtime_error("Failed to create deferred pipeline state");
        }

        Logger::Info("GeometryPass: Deferred pipeline state created (G-Buffer MRT)");
    }

    ComPtr<ID3DBlob> GeometryPass::CompileShader(const std::wstring& filepath,
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

    void GeometryPass::UpdateBuffers(ID3D12Device* device) {
        if (cachedVertices.empty() || cachedIndices.empty()) {
            return;
        }

        try {
            // 頂点バッファ作成
            if (!vertexBuffer) {
                vertexBuffer = std::make_unique<Buffer>();
            }
            vertexBuffer->Initialize(
                device,
                static_cast<uint32_t>(cachedVertices.size() * sizeof(Vertex)),
                BufferType::Vertex,
                D3D12_HEAP_TYPE_UPLOAD
            );
            vertexBuffer->Upload(cachedVertices.data(), static_cast<uint32_t>(cachedVertices.size() * sizeof(Vertex)));

            // インデックスバッファ作成
            if (!indexBuffer) {
                indexBuffer = std::make_unique<Buffer>();
            }
            indexBuffer->Initialize(
                device,
                static_cast<uint32_t>(cachedIndices.size() * sizeof(uint32_t)),
                BufferType::Index,
                D3D12_HEAP_TYPE_UPLOAD
            );
            indexBuffer->Upload(cachedIndices.data(), static_cast<uint32_t>(cachedIndices.size() * sizeof(uint32_t)));

            buffersDirty = false;
            Logger::Info("GeometryPass: Buffers updated successfully (vertices: %zu, indices: %zu)", 
                        cachedVertices.size(), cachedIndices.size());
        }
        catch (const std::exception& e) {
            Logger::Error("GeometryPass: Failed to update buffers: %s", e.what());
        }
    }

    void GeometryPass::InitializeBindlessHeap(ID3D12Device* device, uint32_t maxTextures) {
        bindlessHeap = std::make_unique<DescriptorHeap>();
        bindlessHeap->Initialize(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, maxTextures, true);
        bindlessEnabled = true;
        nextTextureIndex = 0;
        Logger::Info("GeometryPass: Bindless heap initialized with capacity: %u", maxTextures);
    }

    uint32_t GeometryPass::AddTextureToBindlessHeap(std::shared_ptr<Texture> texture, ID3D12Device* device) {
        if (!bindlessEnabled || !bindlessHeap) {
            Logger::Warning("GeometryPass: Bindless heap not initialized");
            return 0;
        }

        // 既に登録済みかチェック
        auto it = textureIndexMap.find(texture);
        if (it != textureIndexMap.end()) {
            return it->second;
        }

        // 新しいインデックスを割り当て
        uint32_t index = nextTextureIndex++;
        textureIndexMap[texture] = index;

        // デスクリプタを割り当ててSRVを作成
        auto handle = bindlessHeap->Allocate();
        texture->CreateSRV(device, handle.cpu);

        Logger::Info("GeometryPass: Added texture to bindless heap at index: %u", index);
        return index;
    }

} // namespace Athena