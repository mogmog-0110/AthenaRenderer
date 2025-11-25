#include "Athena/RenderGraph/RenderGraph.h"
#include "Athena/RenderGraph/RenderGraphBuilder.h"
#include "Athena/RenderGraph/GeometryPass.h"
#include "Athena/RenderGraph/LightingPass.h"
#include "Athena/RenderGraph/ToneMappingPass.h"
#include "Athena/RenderGraph/RenderPass.h"
#include "Athena/Core/Device.h"
#include "Athena/Core/DescriptorHeap.h"
#include "Athena/Resources/Texture.h"
#include "Athena/Utils/Logger.h"
#include <memory>
#include <vector>

namespace Athena {

/**
 * @brief RenderGraphを使用した基本的なレンダリングパイプラインの例
 */
class RenderGraphExample {
public:
    RenderGraphExample() = default;
    ~RenderGraphExample() = default;

    /**
     * @brief RenderGraphの初期化
     */
    bool Initialize(std::shared_ptr<Device> device, uint32_t width, uint32_t height) {
        this->device = device;
        this->width = width;
        this->height = height;

        try {
            // RenderGraphBuilderを作成
            builder = std::make_unique<RenderGraphBuilder>();

            // 各レンダーパスを作成
            geometryPass = std::make_unique<GeometryPass>();
            lightingPass = std::make_unique<LightingPass>();
            toneMappingPass = std::make_unique<ToneMappingPass>();

            // G-Bufferテクスチャを作成
            CreateGBufferTextures();
            
            // G-Bufferデスクリプタヒープを作成
            CreateGBufferDescriptorHeaps();

            // RenderGraphを構築
            BuildRenderGraph();

            Logger::Info("RenderGraphExample: Initialization completed");
            return true;
        }
        catch (const std::exception& e) {
            Logger::Error("RenderGraphExample: Initialization failed: %s", e.what());
            return false;
        }
    }

    /**
     * @brief フレームレンダリング
     */
    void Render(ID3D12GraphicsCommandList* commandList, 
                ID3D12DescriptorHeap* srvHeap,
                D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle,
                D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle,
                bool useDeferredRendering = false) {
        
        if (!renderGraph) {
            Logger::Warning("RenderGraphExample: RenderGraph not initialized");
            return;
        }

        try {
            // 実行データを準備
            PassExecuteData executeData;
            executeData.commandList = commandList;
            executeData.srvHeap = srvHeap;

            if (useDeferredRendering) {
                // ディファードレンダリングパイプライン
                Logger::Info("RenderGraphExample: Executing Deferred Rendering Pipeline");
                
                // G-BufferモードでGeometryPass実行
                if (geometryPass) {
                    geometryPass->SetRenderMode(GeometryPass::RenderMode::Deferred);
                    geometryPass->SetGBufferTargets(gbufferAlbedo, gbufferNormal, gbufferMaterial);
                    geometryPass->SetGBufferDepth(gbufferDepth);
                    geometryPass->SetGBufferRTVHandles(gbufferRTVHandles);
                    geometryPass->SetGBufferDSVHandle(gbufferDSVHandle);
                    Logger::Info("RenderGraphExample: Executing GeometryPass (G-Buffer generation)");
                    geometryPass->Execute(executeData);
                }
                
                // LightingPass実行（G-Bufferからライティング）
                if (lightingPass) {
                    // レンダーターゲットを最終出力（スワップチェーン）に設定
                    commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);
                    
                    // G-Buffer SRVヒープに変更
                    PassExecuteData lightingData = executeData;
                    lightingData.srvHeap = gbufferSRVHeap->GetD3D12DescriptorHeap();
                    
                    lightingPass->SetGBuffer(gbufferAlbedo, gbufferNormal, gbufferDepth);
                    Logger::Info("RenderGraphExample: Executing LightingPass (Deferred Lighting)");
                    lightingPass->Execute(lightingData);
                }
            } else {
                // フォワードレンダリングパイプライン
                Logger::Info("RenderGraphExample: Executing Forward Rendering Pipeline");
                
                // フォワードレンダリング用のレンダーターゲット設定
                commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);
                
                if (geometryPass) {
                    geometryPass->SetRenderMode(GeometryPass::RenderMode::Forward);
                    Logger::Info("RenderGraphExample: Executing GeometryPass (Forward rendering)");
                    geometryPass->Execute(executeData);
                }
            }

            Logger::Info("RenderGraphExample: Frame rendered");
        }
        catch (const std::exception& e) {
            Logger::Error("RenderGraphExample: Render failed: %s", e.what());
        }
    }

    /**
     * @brief シーンデータの設定
     */
    void SetSceneData(const Matrix4x4& world, const Matrix4x4& view, const Matrix4x4& proj,
                     const Vector3& cameraPos, const Vector3& lightDir, const Vector3& lightColor) {
        if (geometryPass) {
            geometryPass->SetTransform(world, view, proj);
            geometryPass->SetCameraPosition(cameraPos);
            geometryPass->SetLight(lightDir, lightColor);
        }

        if (lightingPass) {
            lightingPass->SetCamera(cameraPos, 1.0f);
            lightingPass->ClearLights();
            lightingPass->AddDirectionalLight(lightDir, lightColor, 1.0f);
        }

        if (toneMappingPass) {
            toneMappingPass->SetExposure(1.0f);
            toneMappingPass->SetGamma(2.2f);
        }
    }

    /**
     * @brief 頂点データの設定
     */
    void SetVertexData(const GeometryPass::Vertex* vertices, uint32_t vertexCount,
                      const uint32_t* indices, uint32_t indexCount) {
        if (geometryPass) {
            geometryPass->SetVertexData(vertices, vertexCount, indices, indexCount);
        }
    }

    /**
     * @brief テクスチャの設定
     */
    void SetTexture(std::shared_ptr<Texture> texture) {
        if (geometryPass) {
            geometryPass->SetTexture(texture);
        }
    }

    /**
     * @brief オブジェクトIDの設定
     */
    void SetObjectID(uint32_t objectID) {
        if (geometryPass) {
            geometryPass->SetObjectID(objectID);
        }
    }

    /**
     * @brief レンダリングモードを設定
     */
    void SetRenderingMode(bool useDeferred) {
        deferredMode = useDeferred;
    }

    /**
     * @brief G-Bufferテクスチャを取得
     */
    std::shared_ptr<Texture> GetGBufferAlbedo() const { return gbufferAlbedo; }
    std::shared_ptr<Texture> GetGBufferNormal() const { return gbufferNormal; }
    std::shared_ptr<Texture> GetGBufferMaterial() const { return gbufferMaterial; }
    std::shared_ptr<Texture> GetGBufferDepth() const { return gbufferDepth; }

private:
    /**
     * @brief RenderGraphを構築
     */
    void BuildRenderGraph() {
        if (!builder) return;

        // 簡易RenderGraphを作成（基本的な初期化のみ）
        renderGraph = std::make_unique<RenderGraph>(device);
        
        // 各パスをRenderGraphに追加してセットアップ
        if (geometryPass && device) {
            // まず独自にSetupを実行
            PassSetupData setupData;
            setupData.device = device->GetD3D12Device();
            geometryPass->Setup(setupData);
            Logger::Info("RenderGraphExample: GeometryPass setup completed");
            
            // RenderGraphにパスを追加（注意：パスの所有権をRenderGraphに移譲しないため、警告は残る）
            // 現在の設計では個別管理を維持し、RenderGraphは主にリソース管理に使用
            Logger::Info("RenderGraphExample: GeometryPass managed independently");
        }
        
        if (lightingPass && device) {
            PassSetupData setupData;
            setupData.device = device->GetD3D12Device();
            lightingPass->Setup(setupData);
            Logger::Info("RenderGraphExample: LightingPass setup completed");
        }
        
        if (toneMappingPass && device) {
            PassSetupData setupData;
            setupData.device = device->GetD3D12Device();
            toneMappingPass->Setup(setupData);
            Logger::Info("RenderGraphExample: ToneMappingPass setup completed");
        }

        Logger::Info("RenderGraphExample: RenderGraph built successfully");
    }

    /**
     * @brief G-Bufferテクスチャを作成
     */
    void CreateGBufferTextures() {
        if (!device) return;

        try {
            // Albedo + Metallic バッファ
            gbufferAlbedo = std::make_shared<Texture>();
            gbufferAlbedo->CreateRenderTarget(
                device->GetD3D12Device(),
                width, height,
                DXGI_FORMAT_R8G8B8A8_UNORM
            );

            // Normal + Roughness バッファ
            gbufferNormal = std::make_shared<Texture>();
            gbufferNormal->CreateRenderTarget(
                device->GetD3D12Device(),
                width, height,
                DXGI_FORMAT_R8G8B8A8_UNORM
            );

            // マテリアルパラメータバッファ
            gbufferMaterial = std::make_shared<Texture>();
            gbufferMaterial->CreateRenderTarget(
                device->GetD3D12Device(),
                width, height,
                DXGI_FORMAT_R8G8B8A8_UNORM
            );

            // 深度バッファ
            gbufferDepth = std::make_shared<Texture>();
            gbufferDepth->CreateDepthStencil(
                device->GetD3D12Device(),
                width, height
            );

            // HDRライティング結果バッファ
            hdrTexture = std::make_shared<Texture>();
            hdrTexture->CreateRenderTarget(
                device->GetD3D12Device(),
                width, height,
                DXGI_FORMAT_R16G16B16A16_FLOAT
            );

            Logger::Info("RenderGraphExample: G-Buffer textures created successfully");
        }
        catch (const std::exception& e) {
            Logger::Error("RenderGraphExample: Failed to create G-Buffer textures: %s", e.what());
        }
    }
    
    /**
     * @brief G-Bufferデスクリプタヒープとビューを作成
     */
    void CreateGBufferDescriptorHeaps() {
        if (!device) return;
        
        try {
            // RTVヒープ作成 (3個のG-Bufferレンツーターゲット用)
            gbufferRTVHeap = std::make_unique<DescriptorHeap>();
            gbufferRTVHeap->Initialize(device->GetD3D12Device(), D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 3, false);
            
            // DSVヒープ作成 (深度バッファ用)
            gbufferDSVHeap = std::make_unique<DescriptorHeap>();
            gbufferDSVHeap->Initialize(device->GetD3D12Device(), D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1, false);
            
            // SRVヒープ作成 (G-Buffer読み取り用)
            gbufferSRVHeap = std::make_unique<DescriptorHeap>();
            gbufferSRVHeap->Initialize(device->GetD3D12Device(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 3, true);
            gbufferSRVGPUStart = gbufferSRVHeap->GetD3D12DescriptorHeap()->GetGPUDescriptorHandleForHeapStart();
            
            // RTVを作成
            auto albedoHandle = gbufferRTVHeap->Allocate();
            gbufferAlbedo->CreateRTV(device->GetD3D12Device(), albedoHandle.cpu);
            gbufferRTVHandles[0] = albedoHandle.cpu;
            
            auto normalHandle = gbufferRTVHeap->Allocate();
            gbufferNormal->CreateRTV(device->GetD3D12Device(), normalHandle.cpu);
            gbufferRTVHandles[1] = normalHandle.cpu;
            
            auto materialHandle = gbufferRTVHeap->Allocate();
            gbufferMaterial->CreateRTV(device->GetD3D12Device(), materialHandle.cpu);
            gbufferRTVHandles[2] = materialHandle.cpu;
            
            // DSVを作成
            auto depthHandle = gbufferDSVHeap->Allocate();
            gbufferDepth->CreateDSV(device->GetD3D12Device(), depthHandle.cpu);
            gbufferDSVHandle = depthHandle.cpu;
            
            // SRVを作成 (LightingPass用)
            auto albedoSRV = gbufferSRVHeap->Allocate();
            gbufferAlbedo->CreateSRV(device->GetD3D12Device(), albedoSRV.cpu);
            
            auto normalSRV = gbufferSRVHeap->Allocate();
            gbufferNormal->CreateSRV(device->GetD3D12Device(), normalSRV.cpu);
            
            auto depthSRV = gbufferSRVHeap->Allocate();
            gbufferDepth->CreateSRV(device->GetD3D12Device(), depthSRV.cpu);
            
            Logger::Info("RenderGraphExample: G-Buffer descriptor heaps and views created successfully");
        }
        catch (const std::exception& e) {
            Logger::Error("RenderGraphExample: Failed to create G-Buffer descriptor heaps: %s", e.what());
        }
    }

private:
    std::shared_ptr<Device> device;
    uint32_t width = 0;
    uint32_t height = 0;

    std::unique_ptr<RenderGraphBuilder> builder;
    std::unique_ptr<RenderGraph> renderGraph;

    // レンダーパス
    std::unique_ptr<GeometryPass> geometryPass;
    std::unique_ptr<LightingPass> lightingPass;
    std::unique_ptr<ToneMappingPass> toneMappingPass;

    // G-Bufferテクスチャ
    std::shared_ptr<Texture> gbufferAlbedo;
    std::shared_ptr<Texture> gbufferNormal;
    std::shared_ptr<Texture> gbufferMaterial;  // 追加したマテリアルバッファ
    std::shared_ptr<Texture> gbufferDepth;
    std::shared_ptr<Texture> hdrTexture;
    
    // G-Bufferデスクリプタヒープとハンドル
    std::unique_ptr<DescriptorHeap> gbufferRTVHeap;
    std::unique_ptr<DescriptorHeap> gbufferDSVHeap;
    std::unique_ptr<DescriptorHeap> gbufferSRVHeap;
    D3D12_CPU_DESCRIPTOR_HANDLE gbufferRTVHandles[3] = {};
    D3D12_CPU_DESCRIPTOR_HANDLE gbufferDSVHandle = {};
    D3D12_GPU_DESCRIPTOR_HANDLE gbufferSRVGPUStart = {};
    
    // レンダリングモード
    bool deferredMode = false;
};

} // namespace Athena

// グローバル関数として公開
bool InitializeRenderGraphExample(std::shared_ptr<Athena::Device> device, uint32_t width, uint32_t height);
void RenderWithRenderGraph(ID3D12GraphicsCommandList* commandList, 
                          ID3D12DescriptorHeap* srvHeap,
                          D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle,
                          D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle,
                          bool useDeferredRendering = false);
void SetRenderGraphMode(bool useDeferred);
void SetRenderGraphSceneData(const Athena::Matrix4x4& world, const Athena::Matrix4x4& view, const Athena::Matrix4x4& proj,
                           const Athena::Vector3& cameraPos, const Athena::Vector3& lightDir, const Athena::Vector3& lightColor);

// グローバルインスタンス
static std::unique_ptr<Athena::RenderGraphExample> g_renderGraphExample;

bool InitializeRenderGraphExample(std::shared_ptr<Athena::Device> device, uint32_t width, uint32_t height) {
    g_renderGraphExample = std::make_unique<Athena::RenderGraphExample>();
    return g_renderGraphExample->Initialize(device, width, height);
}

void RenderWithRenderGraph(ID3D12GraphicsCommandList* commandList, 
                          ID3D12DescriptorHeap* srvHeap,
                          D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle,
                          D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle,
                          bool useDeferredRendering) {
    if (g_renderGraphExample) {
        g_renderGraphExample->Render(commandList, srvHeap, rtvHandle, dsvHandle, useDeferredRendering);
    }
}

void SetRenderGraphMode(bool useDeferred) {
    if (g_renderGraphExample) {
        g_renderGraphExample->SetRenderingMode(useDeferred);
    }
}

void SetRenderGraphSceneData(const Athena::Matrix4x4& world, const Athena::Matrix4x4& view, const Athena::Matrix4x4& proj,
                           const Athena::Vector3& cameraPos, const Athena::Vector3& lightDir, const Athena::Vector3& lightColor) {
    if (g_renderGraphExample) {
        g_renderGraphExample->SetSceneData(world, view, proj, cameraPos, lightDir, lightColor);
    }
}

void SetRenderGraphVertexData(const void* vertices, uint32_t vertexCount, const uint32_t* indices, uint32_t indexCount) {
    if (g_renderGraphExample) {
        // main.cppのTestVertex形式からGeometryPass::Vertex形式に変換
        struct TestVertex {
            Athena::Vector3 position;
            Athena::Vector3 normal;
            float u, v;
        };
        
        const TestVertex* testVertices = static_cast<const TestVertex*>(vertices);
        
        std::vector<Athena::GeometryPass::Vertex> convertedVertices;
        convertedVertices.reserve(vertexCount);
        
        for (uint32_t i = 0; i < vertexCount; ++i) {
            Athena::GeometryPass::Vertex vertex;
            vertex.position = testVertices[i].position;
            vertex.normal = testVertices[i].normal;
            vertex.texcoord.x = testVertices[i].u;
            vertex.texcoord.y = testVertices[i].v;
            convertedVertices.push_back(vertex);
        }
        
        g_renderGraphExample->SetVertexData(convertedVertices.data(), vertexCount, indices, indexCount);
    }
}

void SetRenderGraphTexture(std::shared_ptr<Athena::Texture> texture) {
    if (g_renderGraphExample) {
        g_renderGraphExample->SetTexture(texture);
    }
}

void SetRenderGraphObjectID(uint32_t objectID) {
    if (g_renderGraphExample) {
        g_renderGraphExample->SetObjectID(objectID);
    }
}