#define NOMINMAX
#include "Athena/RenderGraph/RenderGraph.h"
#include "Athena/Core/Device.h"
#include "Athena/Resources/Texture.h"
#include "Athena/Resources/Buffer.h"
#include "Athena/Utils/Logger.h"
#include <algorithm>
#include <sstream>
#include <chrono>
#include <queue>
#include <limits>

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
        
        // ステップ5: リソースライフタイム解析
        AnalyzeResourceLifetime();
        
        // ステップ5.5: リソース配置最適化
        OptimizeResourceAllocation();
        
        // ステップ5.7: リソースバリア解析
        AnalyzeResourceBarriers();
        
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
            
            // コマンドリストを取得（仮の実装）
            ID3D12GraphicsCommandList* commandList = executeData.commandList;
            if (commandList) {
                // パス実行前のリソースバリアを挿入
                InsertResourceBarriers(commandList, passInfo.preBarriers);
            }
            
            try {
                passInfo.pass->Execute(executeData);
                stats.executedPasses++;
                
                // パス実行後のリソースバリアを挿入
                if (commandList) {
                    InsertResourceBarriers(commandList, passInfo.postBarriers);
                }
                
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
        executionOrder.clear();
        
        // 有効なパスのリスト作成
        std::vector<uint32_t> enabledPasses;
        for (size_t i = 0; i < passes.size(); ++i) {
            if (passes[i].enabled && passes[i].pass) {
                enabledPasses.push_back(static_cast<uint32_t>(i));
            }
        }
        
        if (enabledPasses.empty()) {
            Logger::Warning("No enabled passes found for dependency analysis");
            return true;
        }
        
        // トポロジカルソートを実行
        if (!TopologicalSort(enabledPasses)) {
            Logger::Error("Failed to perform topological sort - cyclic dependency detected");
            return false;
        }
        
        Logger::Info("Dependency analysis completed, execution order: {} passes", executionOrder.size());
        return true;
    }

    bool RenderGraph::TopologicalSort(const std::vector<uint32_t>& enabledPasses) {
        // カーンのアルゴリズムでトポロジカルソートを実行
        std::unordered_map<uint32_t, uint32_t> inDegree;  // 各パスの入次数
        std::unordered_map<uint32_t, std::vector<uint32_t>> adjacencyList;  // 隣接リスト
        
        // 初期化
        for (uint32_t passIndex : enabledPasses) {
            inDegree[passIndex] = 0;
            adjacencyList[passIndex] = {};
        }
        
        // 依存関係を構築
        BuildDependencyGraph(enabledPasses, inDegree, adjacencyList);
        
        // 入次数が0のパスをキューに追加
        std::queue<uint32_t> queue;
        for (uint32_t passIndex : enabledPasses) {
            if (inDegree[passIndex] == 0) {
                queue.push(passIndex);
            }
        }
        
        // トポロジカルソートの実行
        while (!queue.empty()) {
            uint32_t currentPass = queue.front();
            queue.pop();
            executionOrder.push_back(currentPass);
            
            // 現在のパスから依存しているパスの入次数を減らす
            for (uint32_t dependentPass : adjacencyList[currentPass]) {
                inDegree[dependentPass]--;
                if (inDegree[dependentPass] == 0) {
                    queue.push(dependentPass);
                }
            }
        }
        
        // 循環依存の検出
        if (executionOrder.size() != enabledPasses.size()) {
            Logger::Error("Cyclic dependency detected in render graph");
            executionOrder.clear();
            return false;
        }
        
        return true;
    }

    void RenderGraph::BuildDependencyGraph(const std::vector<uint32_t>& enabledPasses,
                                          std::unordered_map<uint32_t, uint32_t>& inDegree,
                                          std::unordered_map<uint32_t, std::vector<uint32_t>>& adjacencyList) {
        
        // 各リソースを書き込むパスと読み込むパスを追跡
        std::unordered_map<uint32_t, uint32_t> resourceWriters;  // リソースID -> 書き込みパス
        std::unordered_map<uint32_t, std::vector<uint32_t>> resourceReaders;  // リソースID -> 読み込みパス一覧
        
        for (uint32_t passIndex : enabledPasses) {
            const PassInfo& passInfo = passes[passIndex];
            
            // 出力リソース（このパスが書き込む）
            for (const ResourceHandle& output : passInfo.outputs) {
                if (output.IsValid()) {
                    uint32_t resourceId = output.GetID();
                    
                    // 以前に同じリソースに書き込んでいるパスがあれば依存関係を追加
                    if (resourceWriters.find(resourceId) != resourceWriters.end()) {
                        uint32_t prevWriterPass = resourceWriters[resourceId];
                        adjacencyList[prevWriterPass].push_back(passIndex);
                        inDegree[passIndex]++;
                    }
                    
                    // このリソースを読み込んでいる全てのパスにも依存関係を追加
                    if (resourceReaders.find(resourceId) != resourceReaders.end()) {
                        for (uint32_t readerPass : resourceReaders[resourceId]) {
                            adjacencyList[readerPass].push_back(passIndex);
                            inDegree[passIndex]++;
                        }
                    }
                    
                    resourceWriters[resourceId] = passIndex;
                }
            }
            
            // 入力リソース（このパスが読み込む）
            for (const ResourceHandle& input : passInfo.inputs) {
                if (input.IsValid()) {
                    uint32_t resourceId = input.GetID();
                    
                    // このリソースを書き込んでいるパスがあれば依存関係を追加
                    if (resourceWriters.find(resourceId) != resourceWriters.end()) {
                        uint32_t writerPass = resourceWriters[resourceId];
                        adjacencyList[writerPass].push_back(passIndex);
                        inDegree[passIndex]++;
                    }
                    
                    // このパスを読み込みパスリストに追加
                    resourceReaders[resourceId].push_back(passIndex);
                }
            }
        }
        
        // 依存関係の総数を計算
        size_t totalDeps = 0;
        for (const auto& passEntry : adjacencyList) {
            totalDeps += passEntry.second.size();
        }
        
        Logger::Info("Built dependency graph with {} dependencies", totalDeps);
    }

    void RenderGraph::CullUnusedPasses() {
        stats.culledPasses = 0;
        
        // 簡単な実装：最終出力に到達しないパスをカル
        // TODO: より複雑な依存関係解析を実装
        
        Logger::Info("Pass culling completed, culled {} passes", stats.culledPasses);
    }

    void RenderGraph::AnalyzeResourceLifetime() {
        // 各リソースの最初と最後の使用パスを計算
        for (auto& [id, resource] : resources) {
            resource.firstPass = 0xFFFFFFFF;
            resource.lastPass = 0xFFFFFFFF;
        }
        
        // 実行順序に基づいてライフタイムを計算
        for (size_t i = 0; i < executionOrder.size(); ++i) {
            uint32_t passIndex = executionOrder[i];
            if (passIndex >= passes.size()) continue;
            
            const PassInfo& passInfo = passes[passIndex];
            
            // 入力リソースの使用を記録
            for (const ResourceHandle& input : passInfo.inputs) {
                if (input.IsValid()) {
                    uint32_t resourceId = input.GetID();
                    auto it = resources.find(resourceId);
                    if (it != resources.end()) {
                        ResourceInfo& resourceInfo = it->second;
                        resourceInfo.firstPass = std::min(resourceInfo.firstPass, passIndex);
                        resourceInfo.lastPass = std::max(resourceInfo.lastPass, passIndex);
                    }
                }
            }
            
            // 出力リソースの使用を記録
            for (const ResourceHandle& output : passInfo.outputs) {
                if (output.IsValid()) {
                    uint32_t resourceId = output.GetID();
                    auto it = resources.find(resourceId);
                    if (it != resources.end()) {
                        ResourceInfo& resourceInfo = it->second;
                        resourceInfo.firstPass = std::min(resourceInfo.firstPass, passIndex);
                        resourceInfo.lastPass = std::max(resourceInfo.lastPass, passIndex);
                    }
                }
            }
        }
        
        // 最終出力リソースのライフタイムを延長
        for (const ResourceHandle& finalOutput : finalOutputs) {
            if (finalOutput.IsValid()) {
                uint32_t resourceId = finalOutput.GetID();
                auto it = resources.find(resourceId);
                if (it != resources.end()) {
                    it->second.lastPass = static_cast<uint32_t>(executionOrder.size());
                }
            }
        }
        
        Logger::Info("Resource lifetime analysis completed");
    }

    void RenderGraph::OptimizeResourceAllocation() {
        // メモリ使用量の最適化
        // リソースのエイリアシング（メモリ再利用）を実行
        
        if (!settings.enableResourceAliasing) {
            Logger::Info("Resource aliasing disabled, skipping optimization");
            return;
        }
        
        // ライフタイムが重複しないリソース同士をエイリアシング可能としてマーク
        std::vector<std::pair<uint32_t, ResourceInfo*>> transientResources;
        
        for (auto& [id, resource] : resources) {
            if (!resource.isExternal && resource.isTransient) {
                transientResources.push_back({id, &resource});
            }
        }
        
        // ライフタイムでソート
        std::sort(transientResources.begin(), transientResources.end(),
            [](const auto& a, const auto& b) {
                return a.second->firstPass < b.second->firstPass;
            });
        
        size_t optimizedResources = 0;
        
        // 簡単な最適化：同じサイズとフォーマットのリソースをエイリアシング
        for (size_t i = 0; i < transientResources.size(); ++i) {
            for (size_t j = i + 1; j < transientResources.size(); ++j) {
                ResourceInfo* resA = transientResources[i].second;
                ResourceInfo* resB = transientResources[j].second;
                
                // ライフタイムが重複しないかチェック
                if (resA->lastPass < resB->firstPass || resB->lastPass < resA->firstPass) {
                    // リソースの互換性をチェック
                    if (ResourcesCompatible(resA->desc, resB->desc)) {
                        // エイリアシング可能
                        optimizedResources++;
                    }
                }
            }
        }
        
        Logger::Info("Resource allocation optimization completed, optimized {} resources", optimizedResources);
    }

    bool RenderGraph::ResourcesCompatible(const ResourceDesc& a, const ResourceDesc& b) const {
        // リソースがエイリアシング可能（メモリ再利用可能）かチェック
        
        // タイプが一致している必要がある
        if (a.type != b.type) {
            return false;
        }
        
        // テクスチャの場合
        if (a.type == ResourceType::Texture2D) {
            return (a.width == b.width) && 
                   (a.height == b.height) && 
                   (a.format == b.format) &&
                   (a.usage == b.usage);
        }
        
        // バッファの場合
        if (a.type == ResourceType::Buffer) {
            return (a.width >= b.width) &&  // サイズが同じかより大きい
                   (a.usage == b.usage);
        }
        
        return false;
    }

    void RenderGraph::AnalyzeResourceBarriers() {
        // 全てのパスのバリアをクリア
        for (auto& passInfo : passes) {
            passInfo.preBarriers.clear();
            passInfo.postBarriers.clear();
        }
        
        // リソース状態を初期化
        for (auto& [id, resource] : resources) {
            resource.currentState = D3D12_RESOURCE_STATE_COMMON;
        }
        
        // 実行順序に従ってバリアを挿入
        for (size_t i = 0; i < executionOrder.size(); ++i) {
            uint32_t passIndex = executionOrder[i];
            if (passIndex >= passes.size()) continue;
            
            PassInfo& passInfo = passes[passIndex];
            
            // 入力リソースのバリア解析
            for (const ResourceHandle& input : passInfo.inputs) {
                if (!input.IsValid()) continue;
                
                uint32_t resourceId = input.GetID();
                auto it = resources.find(resourceId);
                if (it == resources.end()) continue;
                
                ResourceInfo& resourceInfo = it->second;
                D3D12_RESOURCE_STATES requiredState = GetResourceStateFromUsage(input.GetDesc().usage, false);
                
                // 状態遷移が必要かチェック
                if (resourceInfo.currentState != requiredState) {
                    ResourceStateTransition transition;
                    transition.resourceId = resourceId;
                    transition.fromState = resourceInfo.currentState;
                    transition.toState = requiredState;
                    transition.passIndex = passIndex;
                    
                    passInfo.preBarriers.push_back(transition);
                    resourceInfo.currentState = requiredState;
                }
            }
            
            // 出力リソースのバリア解析
            for (const ResourceHandle& output : passInfo.outputs) {
                if (!output.IsValid()) continue;
                
                uint32_t resourceId = output.GetID();
                auto it = resources.find(resourceId);
                if (it == resources.end()) continue;
                
                ResourceInfo& resourceInfo = it->second;
                D3D12_RESOURCE_STATES requiredState = GetResourceStateFromUsage(output.GetDesc().usage, true);
                
                // 状態遷移が必要かチェック
                if (resourceInfo.currentState != requiredState) {
                    ResourceStateTransition transition;
                    transition.resourceId = resourceId;
                    transition.fromState = resourceInfo.currentState;
                    transition.toState = requiredState;
                    transition.passIndex = passIndex;
                    
                    passInfo.preBarriers.push_back(transition);
                    resourceInfo.currentState = requiredState;
                }
            }
        }
        
        size_t totalBarriers = 0;
        for (const auto& passInfo : passes) {
            totalBarriers += passInfo.preBarriers.size() + passInfo.postBarriers.size();
        }
        
        Logger::Info("Resource barrier analysis completed, inserted {} barriers", totalBarriers);
    }

    D3D12_RESOURCE_STATES RenderGraph::GetResourceStateFromUsage(ResourceUsage usage, bool isWrite) const {
        // ResourceUsage列挙体の値に基づいて適切な状態を返す
        if (HasUsage(usage, ResourceUsage::RenderTarget)) {
            return D3D12_RESOURCE_STATE_RENDER_TARGET;
        }
        
        if (HasUsage(usage, ResourceUsage::DepthStencil)) {
            return isWrite ? D3D12_RESOURCE_STATE_DEPTH_WRITE : D3D12_RESOURCE_STATE_DEPTH_READ;
        }
        
        if (HasUsage(usage, ResourceUsage::ShaderResource)) {
            return D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
        }
        
        if (HasUsage(usage, ResourceUsage::UnorderedAccess)) {
            return D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
        }
        
        if (HasUsage(usage, ResourceUsage::CopyDestination)) {
            return D3D12_RESOURCE_STATE_COPY_DEST;
        }
        
        if (HasUsage(usage, ResourceUsage::CopySource)) {
            return D3D12_RESOURCE_STATE_COPY_SOURCE;
        }
        
        // デフォルト
        return D3D12_RESOURCE_STATE_COMMON;
    }

    void RenderGraph::InsertResourceBarriers(ID3D12GraphicsCommandList* commandList,
                                           const std::vector<ResourceStateTransition>& barriers) {
        if (barriers.empty() || !commandList) {
            return;
        }
        
        std::vector<D3D12_RESOURCE_BARRIER> d3dBarriers;
        d3dBarriers.reserve(barriers.size());
        
        for (const auto& transition : barriers) {
            auto it = resources.find(transition.resourceId);
            if (it == resources.end()) {
                continue;
            }
            
            const ResourceInfo& resourceInfo = it->second;
            ID3D12Resource* d3dResource = nullptr;
            
            // 実際のD3D12リソースを取得
            if (resourceInfo.texture) {
                d3dResource = resourceInfo.texture->GetD3D12Resource();
            } else if (resourceInfo.buffer) {
                d3dResource = resourceInfo.buffer->GetD3D12Resource();
            }
            
            if (!d3dResource) {
                continue;
            }
            
            // D3D12バリア構造体を構築
            D3D12_RESOURCE_BARRIER barrier = {};
            barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
            barrier.Transition.pResource = d3dResource;
            barrier.Transition.StateBefore = transition.fromState;
            barrier.Transition.StateAfter = transition.toState;
            barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
            
            // 同じ状態への遷移は不要
            if (barrier.Transition.StateBefore != barrier.Transition.StateAfter) {
                d3dBarriers.push_back(barrier);
            }
        }
        
        // バリアを実行
        if (!d3dBarriers.empty()) {
            commandList->ResourceBarrier(static_cast<UINT>(d3dBarriers.size()), d3dBarriers.data());
            
            Logger::Info("Inserted {} resource barriers", d3dBarriers.size());
        }
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