#include "Athena/RenderGraph/RenderGraph.h"
#include "Athena/Core/Device.h"
#include "Athena/Resources/Texture.h"
#include "Athena/Resources/Buffer.h"
#include "Athena/Utils/Logger.h"
#include <algorithm>
#include <sstream>
#include <chrono>

namespace Athena {

    RenderGraph::RenderGraph(std::shared_ptr<Device> device)
        : device(device) {
        Logger::Info("RenderGraph initialized");
    }

    RenderGraph::~RenderGraph() {
        Clear();
        Logger::Info("RenderGraph destroyed");
    }

    void RenderGraph::Clear() {
        passes.clear();
        resources.clear();
        finalOutputs.clear();
        executionOrder.clear();
        texturePool.clear();
        bufferPool.clear();
        nextResourceId = 1;
        
        // 統計をリセット
        stats = RenderGraphStats{};
        
        Logger::Info("RenderGraph cleared");
    }

    uint32_t RenderGraph::AddPass(std::unique_ptr<RenderPass> pass) {
        if (!pass) {
            Logger::Error("Cannot add null pass to RenderGraph");
            return 0xFFFFFFFF;
        }

        uint32_t passIndex = static_cast<uint32_t>(passes.size());
        
        PassInfo passInfo;
        passInfo.pass = std::move(pass);
        passInfo.passIndex = passIndex;
        passInfo.enabled = true;
        
        passes.push_back(std::move(passInfo));
        
        Logger::Info("Added pass '{}' at index {}", passes.back().pass->GetName(), passIndex);
        return passIndex;
    }

    void RenderGraph::RegisterExternalResource(const ResourceHandle& handle, std::shared_ptr<Texture> resource) {
        if (!handle.IsValid() || !resource) {
            Logger::Error("Invalid handle or resource for external texture registration");
            return;
        }

        ResourceInfo info;
        info.handle = handle;
        info.desc = handle.GetDesc();
        info.texture = resource;
        info.isExternal = true;
        info.isTransient = false;
        
        resources[handle.GetID()] = std::move(info);
        
        Logger::Info("Registered external texture resource: {}", handle.GetName());
    }

    void RenderGraph::RegisterExternalResource(const ResourceHandle& handle, std::shared_ptr<Buffer> resource) {
        if (!handle.IsValid() || !resource) {
            Logger::Error("Invalid handle or resource for external buffer registration");
            return;
        }

        ResourceInfo info;
        info.handle = handle;
        info.desc = handle.GetDesc();
        info.buffer = resource;
        info.isExternal = true;
        info.isTransient = false;
        
        resources[handle.GetID()] = std::move(info);
        
        Logger::Info("Registered external buffer resource: {}", handle.GetName());
    }

    void RenderGraph::SetFinalOutput(const ResourceHandle& handle) {
        if (!handle.IsValid()) {
            Logger::Error("Cannot set invalid handle as final output");
            return;
        }
        
        finalOutputs.insert(handle);
        Logger::Info("Set final output: {}", handle.GetName());
    }

    bool RenderGraph::Compile() {
        auto startTime = std::chrono::high_resolution_clock::now();
        
        Logger::Info("Compiling RenderGraph with {} passes", passes.size());
        
        // ステップ1: バリデーション
        if (settings.enableValidation && !ValidateGraph()) {
            Logger::Error("RenderGraph validation failed");
            return false;
        }
        
        // ステップ2: パスのセットアップを実行
        for (auto& passInfo : passes) {
            if (!passInfo.pass) continue;
            
            // TODO: RenderGraphBuilderを実装してからSetupを呼び出す
            // passInfo.setupData.builder = &builder;
            // passInfo.pass->Setup(passInfo.setupData);
        }
        
        // ステップ3: 依存関係解析
        if (!AnalyzeDependencies()) {
            Logger::Error("Dependency analysis failed");
            return false;
        }
        
        // ステップ4: 未使用パス除去
        if (settings.enablePassCulling) {
            CullUnusedPasses();
        }
        
        // ステップ5: リソースライフタイム計算
        CalculateResourceLifetimes();
        
        // ステップ6: 一時リソース配置
        if (!AllocateTransientResources()) {
            Logger::Error("Transient resource allocation failed");
            return false;
        }
        
        // 統計更新
        auto endTime = std::chrono::high_resolution_clock::now();
        stats.compileTime = std::chrono::duration<float>(endTime - startTime).count();
        stats.totalPasses = static_cast<uint32_t>(passes.size());
        stats.totalResources = static_cast<uint32_t>(resources.size());
        
        // 外部・一時リソース数を計算
        stats.externalResources = 0;
        stats.transientResources = 0;
        for (const auto& [id, resource] : resources) {
            if (resource.isExternal) {
                stats.externalResources++;
            } else if (resource.isTransient) {
                stats.transientResources++;
            }
        }
        
        Logger::Info("RenderGraph compilation completed in {:.3f}ms", stats.compileTime * 1000.0f);
        return true;
    }

    bool RenderGraph::Execute(RenderContext* renderContext) {
        if (!renderContext) {
            Logger::Error("Cannot execute RenderGraph with null RenderContext");
            return false;
        }
        
        auto startTime = std::chrono::high_resolution_clock::now();
        
        Logger::Info("Executing RenderGraph with {} passes", executionOrder.size());
        
        stats.executedPasses = 0;
        
        // 実行順序に従ってパスを実行
        for (uint32_t passIndex : executionOrder) {
            if (passIndex >= passes.size()) {
                Logger::Error("Invalid pass index in execution order: {}", passIndex);
                continue;
            }
            
            PassInfo& passInfo = passes[passIndex];
            if (!passInfo.enabled || !passInfo.pass) {
                continue;
            }
            
            // PassExecuteDataを構築
            PassExecuteData executeData;
            executeData.renderContext = renderContext;
            executeData.floatParams = passInfo.setupData.floatParams;
            executeData.intParams = passInfo.setupData.intParams;
            executeData.boolParams = passInfo.setupData.boolParams;
            
            // 入力・出力リソースを設定
            // TODO: リソースハンドルから実際のリソースマッピングを設定
            
            try {
                passInfo.pass->Execute(executeData);
                stats.executedPasses++;
                
                Logger::Info("Executed pass '{}' ({})", 
                    passInfo.pass->GetName(), passInfo.passIndex);
            }
            catch (const std::exception& e) {
                Logger::Error("Pass '{}' execution failed: {}", 
                    passInfo.pass->GetName(), e.what());
                return false;
            }
        }
        
        auto endTime = std::chrono::high_resolution_clock::now();
        stats.executeTime = std::chrono::duration<float>(endTime - startTime).count();
        
        Logger::Info("RenderGraph execution completed in {:.3f}ms", stats.executeTime * 1000.0f);
        return true;
    }

    const ResourceInfo* RenderGraph::GetResourceInfo(const ResourceHandle& handle) const {
        if (!handle.IsValid()) {
            return nullptr;
        }
        
        auto it = resources.find(handle.GetID());
        return (it != resources.end()) ? &it->second : nullptr;
    }

    void RenderGraph::DumpDebugInfo() const {
        Logger::Info("=== RenderGraph Debug Info ===");
        Logger::Info("Passes: {}", passes.size());
        Logger::Info("Resources: {}", resources.size());
        Logger::Info("Execution Order: {}", executionOrder.size());
        Logger::Info("Final Outputs: {}", finalOutputs.size());
        
        Logger::Info("--- Passes ---");
        for (size_t i = 0; i < passes.size(); ++i) {
            const auto& passInfo = passes[i];
            if (passInfo.pass) {
                Logger::Info("  [{}] {} ({})", i, passInfo.pass->GetName(),
                    passInfo.enabled ? "enabled" : "disabled");
            }
        }
        
        Logger::Info("--- Resources ---");
        for (const auto& [id, resource] : resources) {
            Logger::Info("  [{}] {} ({}x{}, {})", 
                id, resource.handle.GetName(),
                resource.desc.width, resource.desc.height,
                resource.isExternal ? "external" : "transient");
        }
        
        Logger::Info("--- Statistics ---");
        Logger::Info("  Compile Time: {:.3f}ms", stats.compileTime * 1000.0f);
        Logger::Info("  Execute Time: {:.3f}ms", stats.executeTime * 1000.0f);
        Logger::Info("  Memory Usage: {} bytes", stats.memoryUsage);
    }

    std::string RenderGraph::ExportGraphviz() const {
        std::stringstream ss;
        ss << "digraph RenderGraph {\n";
        ss << "  rankdir=TB;\n";
        ss << "  node [shape=box];\n";
        
        // パスノード
        for (size_t i = 0; i < passes.size(); ++i) {
            const auto& passInfo = passes[i];
            if (passInfo.pass) {
                ss << "  pass" << i << " [label=\"" << passInfo.pass->GetName() << "\"];\n";
            }
        }
        
        // リソースノード
        for (const auto& [id, resource] : resources) {
            ss << "  res" << id << " [label=\"" << resource.handle.GetName() 
               << "\", shape=ellipse, color=" << (resource.isExternal ? "blue" : "red") << "];\n";
        }
        
        // エッジ（依存関係）
        // TODO: 実際の依存関係を追加
        
        ss << "}\n";
        return ss.str();
    }

    // プライベートメソッドの実装

    ResourceHandle RenderGraph::CreateResource(const ResourceDesc& desc, const std::string& name) {
        uint32_t id = GenerateResourceId();
        
        // プライベートコンストラクタを呼び出すためにfriendクラスからアクセス
        ResourceHandle handle;
        handle.id = id;
        handle.name = name;
        handle.desc = desc;
        
        ResourceInfo info;
        info.handle = handle;
        info.desc = desc;
        info.isExternal = false;
        info.isTransient = true;
        
        resources[id] = std::move(info);
        
        return handle;
    }

    bool RenderGraph::AnalyzeDependencies() {
        // 簡単な実装：パスインデックス順に実行
        executionOrder.clear();
        
        for (size_t i = 0; i < passes.size(); ++i) {
            if (passes[i].enabled && passes[i].pass) {
                executionOrder.push_back(static_cast<uint32_t>(i));
            }
        }
        
        Logger::Info("Dependency analysis completed, execution order: {} passes", executionOrder.size());
        return true;
    }

    void RenderGraph::CullUnusedPasses() {
        stats.culledPasses = 0;
        
        // 簡単な実装：最終出力に到達しないパスをカル
        // TODO: より複雑な依存関係解析を実装
        
        Logger::Info("Pass culling completed, culled {} passes", stats.culledPasses);
    }

    void RenderGraph::CalculateResourceLifetimes() {
        // 各リソースの最初と最後の使用パスを計算
        for (auto& [id, resource] : resources) {
            resource.firstPass = 0xFFFFFFFF;
            resource.lastPass = 0xFFFFFFFF;
            
            // TODO: パスの入出力からライフタイムを計算
        }
        
        Logger::Info("Resource lifetime calculation completed");
    }

    bool RenderGraph::AllocateTransientResources() {
        // 一時リソースの実際のオブジェクトを作成
        for (auto& [id, resource] : resources) {
            if (resource.isExternal) {
                continue; // 外部リソースはスキップ
            }
            
            try {
                if (resource.desc.type == ResourceType::Texture2D) {
                    resource.texture = CreateTexture(resource.desc);
                } else if (resource.desc.type == ResourceType::Buffer) {
                    resource.buffer = CreateBuffer(resource.desc);
                }
                
                if (!resource.texture && !resource.buffer) {
                    Logger::Error("Failed to create resource: {}", resource.handle.GetName());
                    return false;
                }
                
            } catch (const std::exception& e) {
                Logger::Error("Exception while creating resource '{}': {}", 
                    resource.handle.GetName(), e.what());
                return false;
            }
        }
        
        Logger::Info("Transient resource allocation completed");
        return true;
    }

    std::shared_ptr<Texture> RenderGraph::CreateTexture(const ResourceDesc& desc) {
        // 既存のTextureクラスを使用してテクスチャを作成
        auto texture = std::make_shared<Texture>();
        
        // TODO: ResourceDescからTextureの初期化パラメータを構築
        // texture->Initialize(device.get(), ...);
        
        return texture;
    }

    std::shared_ptr<Buffer> RenderGraph::CreateBuffer(const ResourceDesc& desc) {
        // 既存のBufferクラスを使用してバッファを作成
        auto buffer = std::make_shared<Buffer>();
        
        // TODO: ResourceDescからBufferの初期化パラメータを構築
        // buffer->Initialize(device.get(), ...);
        
        return buffer;
    }

    std::shared_ptr<Texture> RenderGraph::AcquireTexture(const ResourceDesc& desc) {
        // プールから適切なテクスチャを探す
        for (auto it = texturePool.begin(); it != texturePool.end(); ++it) {
            auto& texture = *it;
            
            // TODO: リソースの互換性チェック
            if (texture && texture->GetWidth() == desc.width && 
                texture->GetHeight() == desc.height &&
                texture->GetFormat() == desc.format) {
                
                auto result = texture;
                texturePool.erase(it);
                return result;
            }
        }
        
        // プールに適切なものがない場合は新規作成
        return CreateTexture(desc);
    }

    std::shared_ptr<Buffer> RenderGraph::AcquireBuffer(const ResourceDesc& desc) {
        // プールから適切なバッファを探す
        for (auto it = bufferPool.begin(); it != bufferPool.end(); ++it) {
            auto& buffer = *it;
            
            // TODO: リソースの互換性チェック
            if (buffer && buffer->GetSize() >= desc.width) {
                auto result = buffer;
                bufferPool.erase(it);
                return result;
            }
        }
        
        // プールに適切なものがない場合は新規作成
        return CreateBuffer(desc);
    }

    void RenderGraph::ReleaseTexture(std::shared_ptr<Texture> texture) {
        if (texture && texturePool.size() < settings.maxTransientResources) {
            texturePool.push_back(texture);
        }
    }

    void RenderGraph::ReleaseBuffer(std::shared_ptr<Buffer> buffer) {
        if (buffer && bufferPool.size() < settings.maxTransientResources) {
            bufferPool.push_back(buffer);
        }
    }

    bool RenderGraph::ValidateGraph() const {
        if (passes.empty()) {
            Logger::Warning("RenderGraph has no passes");
            return true; // 空のグラフは有効
        }
        
        // 各パスの検証
        for (const auto& passInfo : passes) {
            if (!ValidatePass(passInfo)) {
                return false;
            }
        }
        
        // 各リソースの検証
        for (const auto& [id, resource] : resources) {
            if (!ValidateResource(resource)) {
                return false;
            }
        }
        
        return true;
    }

    bool RenderGraph::ValidatePass(const PassInfo& passInfo) const {
        if (!passInfo.pass) {
            Logger::Error("PassInfo contains null pass");
            return false;
        }
        
        if (passInfo.pass->GetName().empty()) {
            Logger::Error("Pass has empty name");
            return false;
        }
        
        return true;
    }

    bool RenderGraph::ValidateResource(const ResourceInfo& resourceInfo) const {
        if (!resourceInfo.handle.IsValid()) {
            Logger::Error("ResourceInfo contains invalid handle");
            return false;
        }
        
        if (resourceInfo.desc.width == 0 || resourceInfo.desc.height == 0) {
            Logger::Error("Resource '{}' has invalid dimensions", resourceInfo.handle.GetName());
            return false;
        }
        
        return true;
    }

} // namespace Athena