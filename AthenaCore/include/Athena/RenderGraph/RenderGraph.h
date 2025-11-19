#pragma once
#include "ResourceHandle.h"
#include "RenderPass.h"
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <functional>

namespace Athena {

    // Forward declarations
    class Device;
    class RenderContext;
    class Texture;
    class Buffer;

    /**
     * @brief リソース状態遷移情報
     */
    struct ResourceStateTransition {
        uint32_t resourceId;
        D3D12_RESOURCE_STATES fromState;
        D3D12_RESOURCE_STATES toState;
        uint32_t passIndex;
    };

    /**
     * @brief リソースの実体情報
     */
    struct ResourceInfo {
        ResourceHandle handle;
        ResourceDesc desc;
        std::shared_ptr<Texture> texture;      // テクスチャリソース
        std::shared_ptr<Buffer> buffer;        // バッファリソース
        bool isExternal = false;               // 外部リソースかどうか
        bool isTransient = true;               // 一時的なリソースかどうか
        uint32_t firstPass = 0xFFFFFFFF;       // 最初に使用されるパス番号
        uint32_t lastPass = 0xFFFFFFFF;        // 最後に使用されるパス番号
        D3D12_RESOURCE_STATES currentState = D3D12_RESOURCE_STATE_COMMON;  // 現在の状態
        
        /**
         * @brief 実際のリソースを取得
         */
        template<typename T>
        std::shared_ptr<T> GetResource() const;
    };

    // テンプレート特殊化
    template<>
    inline std::shared_ptr<Texture> ResourceInfo::GetResource<Texture>() const {
        return texture;
    }

    template<>
    inline std::shared_ptr<Buffer> ResourceInfo::GetResource<Buffer>() const {
        return buffer;
    }

    /**
     * @brief パス実行情報
     */
    struct PassInfo {
        std::unique_ptr<RenderPass> pass;
        uint32_t passIndex;
        std::vector<ResourceHandle> inputs;    // 入力リソース
        std::vector<ResourceHandle> outputs;   // 出力リソース
        std::vector<ResourceStateTransition> preBarriers;   // パス実行前のバリア
        std::vector<ResourceStateTransition> postBarriers;  // パス実行後のバリア
        PassSetupData setupData;
        bool enabled = true;
    };

    /**
     * @brief レンダーグラフの設定
     */
    struct RenderGraphSettings {
        bool enableResourceAliasing = true;     // リソースエイリアシング（メモリ最適化）
        bool enablePassCulling = true;          // 未使用パスの除去
        bool enableValidation = true;           // バリデーション有効化
        uint32_t maxTransientResources = 1024;  // 最大一時リソース数
    };

    /**
     * @brief レンダーグラフ統計情報
     */
    struct RenderGraphStats {
        uint32_t totalPasses = 0;
        uint32_t executedPasses = 0;
        uint32_t culledPasses = 0;
        uint32_t totalResources = 0;
        uint32_t transientResources = 0;
        uint32_t externalResources = 0;
        size_t memoryUsage = 0;                 // バイト単位
        float compileTime = 0.0f;               // 秒
        float executeTime = 0.0f;               // 秒
    };

    /**
     * @brief RenderGraph - レンダリングパスの依存関係を管理し、実行を制御
     * 
     * RenderGraphの主な機能：
     * 1. パスの登録と依存関係の管理
     * 2. リソースライフタイムの自動管理
     * 3. メモリ使用量の最適化
     * 4. パスの実行順序決定
     * 5. 未使用パスの自動除去
     */
    class RenderGraph {
    public:
        explicit RenderGraph(std::shared_ptr<Device> device);
        ~RenderGraph();

        /**
         * @brief 設定を更新
         */
        void SetSettings(const RenderGraphSettings& settings) { this->settings = settings; }
        const RenderGraphSettings& GetSettings() const { return settings; }

        /**
         * @brief グラフをクリア（全パス・リソースを削除）
         */
        void Clear();

        /**
         * @brief パスを追加
         * @param pass 追加するパス（unique_ptr）
         * @return パスインデックス
         */
        uint32_t AddPass(std::unique_ptr<RenderPass> pass);

        /**
         * @brief 外部リソースを登録
         * @param handle 外部リソースハンドル
         * @param resource 実際のリソース
         */
        void RegisterExternalResource(const ResourceHandle& handle, std::shared_ptr<Texture> resource);
        void RegisterExternalResource(const ResourceHandle& handle, std::shared_ptr<Buffer> resource);

        /**
         * @brief 最終出力リソースを設定
         * 
         * グラフの最終的な出力として使用するリソースを指定。
         * これによりバックワード依存関係解析が可能になる。
         * 
         * @param handle 最終出力リソースのハンドル
         */
        void SetFinalOutput(const ResourceHandle& handle);

        /**
         * @brief グラフをコンパイル
         * 
         * 1. 依存関係の解析
         * 2. 実行順序の決定
         * 3. リソースライフタイムの計算
         * 4. メモリ最適化
         * 5. 未使用パスの除去
         * 
         * @return コンパイル成功時true
         */
        bool Compile();

        /**
         * @brief グラフを実行
         * @param renderContext レンダリングコンテキスト
         * @return 実行成功時true
         */
        bool Execute(RenderContext* renderContext);

        /**
         * @brief 統計情報を取得
         */
        const RenderGraphStats& GetStats() const { return stats; }

        /**
         * @brief リソース情報を取得
         */
        const ResourceInfo* GetResourceInfo(const ResourceHandle& handle) const;

        /**
         * @brief 実際のリソースを取得
         */
        template<typename T>
        std::shared_ptr<T> GetResource(const ResourceHandle& handle) const {
            const ResourceInfo* info = GetResourceInfo(handle);
            return info ? info->GetResource<T>() : nullptr;
        }

        /**
         * @brief デバッグ情報を出力
         */
        void DumpDebugInfo() const;

        /**
         * @brief グラフをGraphviz形式でエクスポート（可視化用）
         */
        std::string ExportGraphviz() const;

        /**
         * @brief 新しいリソースを作成（RenderGraphBuilder用）
         */
        ResourceHandle CreateResource(const ResourceDesc& desc, const std::string& name);

    private:
        friend class RenderGraphBuilder;

        std::shared_ptr<Device> device;
        RenderGraphSettings settings;
        RenderGraphStats stats;

        // パス管理
        std::vector<PassInfo> passes;
        std::vector<uint32_t> executionOrder;      // 実行順序

        // リソース管理
        std::unordered_map<uint32_t, ResourceInfo> resources;
        std::unordered_set<ResourceHandle> finalOutputs;
        uint32_t nextResourceId = 1;

        // 一時リソースプール
        std::vector<std::shared_ptr<Texture>> texturePool;
        std::vector<std::shared_ptr<Buffer>> bufferPool;

        /**
         * @brief 新しいリソースIDを生成
         */
        uint32_t GenerateResourceId() { return nextResourceId++; }

        /**
         * @brief 依存関係を解析し、実行順序を決定
         */
        bool AnalyzeDependencies();

        /**
         * @brief トポロジカルソート（カーンのアルゴリズム）
         */
        bool TopologicalSort(const std::vector<uint32_t>& enabledPasses);

        /**
         * @brief 依存関係グラフを構築
         */
        void BuildDependencyGraph(const std::vector<uint32_t>& enabledPasses,
                                 std::unordered_map<uint32_t, uint32_t>& inDegree,
                                 std::unordered_map<uint32_t, std::vector<uint32_t>>& adjacencyList);

        /**
         * @brief 未使用パスを除去
         */
        void CullUnusedPasses();

        /**
         * @brief リソースライフタイムを解析
         */
        void AnalyzeResourceLifetime();

        /**
         * @brief リソース配置を最適化
         */
        void OptimizeResourceAllocation();

        /**
         * @brief リソースバリアを解析・挿入
         */
        void AnalyzeResourceBarriers();

        /**
         * @brief リソースの互換性をチェック
         */
        bool ResourcesCompatible(const ResourceDesc& a, const ResourceDesc& b) const;

        /**
         * @brief 使用方法からリソース状態を取得
         */
        D3D12_RESOURCE_STATES GetResourceStateFromUsage(ResourceUsage usage, bool isWrite) const;

        /**
         * @brief リソースバリアをコマンドリストに挿入
         */
        void InsertResourceBarriers(ID3D12GraphicsCommandList* commandList,
                                   const std::vector<ResourceStateTransition>& barriers);

        /**
         * @brief 一時リソースを作成・配置
         */
        bool AllocateTransientResources();

        /**
         * @brief 実際のリソースオブジェクトを作成
         */
        std::shared_ptr<Texture> CreateTexture(const ResourceDesc& desc);
        std::shared_ptr<Buffer> CreateBuffer(const ResourceDesc& desc);

        /**
         * @brief リソースプールから取得
         */
        std::shared_ptr<Texture> AcquireTexture(const ResourceDesc& desc);
        std::shared_ptr<Buffer> AcquireBuffer(const ResourceDesc& desc);

        /**
         * @brief リソースプールに返却
         */
        void ReleaseTexture(std::shared_ptr<Texture> texture);
        void ReleaseBuffer(std::shared_ptr<Buffer> buffer);

        /**
         * @brief バリデーション
         */
        bool ValidateGraph() const;
        bool ValidatePass(const PassInfo& passInfo) const;
        bool ValidateResource(const ResourceInfo& resourceInfo) const;
    };

} // namespace Athena