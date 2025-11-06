#pragma once
#include "RenderGraph.h"
#include "ResourceHandle.h"
#include "RenderPass.h"

namespace Athena {

    /**
     * @brief RenderGraphの構築を簡単にするビルダークラス
     * 
     * RenderGraphBuilderは、複雑なグラフ構築を直感的なAPIで可能にする。
     * パスの追加、リソースの宣言、依存関係の定義を流れるようなインターフェースで提供。
     */
    class RenderGraphBuilder {
    public:
        RenderGraphBuilder() = default;
        explicit RenderGraphBuilder(RenderGraph* graph);
        ~RenderGraphBuilder() = default;

        /**
         * @brief 一時的なテクスチャリソースを作成
         * @param name リソース名
         * @param width 幅
         * @param height 高さ
         * @param format フォーマット
         * @param usage 使用方法
         * @return リソースハンドル
         */
        ResourceHandle CreateTexture(
            const std::string& name,
            uint32_t width,
            uint32_t height,
            DXGI_FORMAT format,
            ResourceUsage usage
        );

        /**
         * @brief 一時的なバッファリソースを作成
         * @param name リソース名
         * @param size サイズ（バイト）
         * @param usage 使用方法
         * @return リソースハンドル
         */
        ResourceHandle CreateBuffer(
            const std::string& name,
            uint32_t size,
            ResourceUsage usage
        );

        /**
         * @brief 外部テクスチャをインポート
         * @param name リソース名
         * @param texture 外部テクスチャ
         * @return リソースハンドル
         */
        ResourceHandle ImportTexture(
            const std::string& name,
            std::shared_ptr<Texture> texture
        );

        /**
         * @brief 外部バッファをインポート
         * @param name リソース名  
         * @param buffer 外部バッファ
         * @return リソースハンドル
         */
        ResourceHandle ImportBuffer(
            const std::string& name,
            std::shared_ptr<Buffer> buffer
        );

        /**
         * @brief リソースを読み取り専用として使用宣言
         * @param handle リソースハンドル
         * @param passName パス名（依存関係解析用）
         * @return 自分自身（チェーン可能）
         */
        RenderGraphBuilder& Read(const ResourceHandle& handle, const std::string& passName = "");

        /**
         * @brief リソースを書き込み用として使用宣言
         * @param handle リソースハンドル  
         * @param passName パス名（依存関係解析用）
         * @return 自分自身（チェーン可能）
         */
        RenderGraphBuilder& Write(const ResourceHandle& handle, const std::string& passName = "");

        /**
         * @brief リソースを読み書き両用として使用宣言
         * @param handle リソースハンドル
         * @param passName パス名（依存関係解析用）
         * @return 自分自身（チェーン可能）
         */
        RenderGraphBuilder& ReadWrite(const ResourceHandle& handle, const std::string& passName = "");

        /**
         * @brief 現在構築中のパスに入力リソースを追加
         * @param name 入力名
         * @param handle リソースハンドル
         * @return 自分自身（チェーン可能）
         */
        RenderGraphBuilder& AddInput(const std::string& name, const ResourceHandle& handle);

        /**
         * @brief 現在構築中のパスに出力リソースを追加
         * @param name 出力名
         * @param handle リソースハンドル
         * @return 自分自身（チェーン可能）
         */
        RenderGraphBuilder& AddOutput(const std::string& name, const ResourceHandle& handle);

        /**
         * @brief パラメータを設定
         */
        RenderGraphBuilder& SetFloat(const std::string& name, float value);
        RenderGraphBuilder& SetInt(const std::string& name, int value);
        RenderGraphBuilder& SetBool(const std::string& name, bool value);

        /**
         * @brief 最終出力リソースを設定
         * @param handle 最終出力リソース
         * @return 自分自身（チェーン可能）
         */
        RenderGraphBuilder& SetFinalOutput(const ResourceHandle& handle);

        /**
         * @brief 現在構築中のパスの情報を取得
         */
        const std::string& GetCurrentPassName() const { return currentPassName; }
        bool IsInPass() const { return inPassContext; }

        /**
         * @brief 便利な組み込みリソース作成メソッド
         */
        ResourceHandle CreateColorTarget(const std::string& name, uint32_t width, uint32_t height, DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM);
        ResourceHandle CreateDepthTarget(const std::string& name, uint32_t width, uint32_t height, DXGI_FORMAT format = DXGI_FORMAT_D32_FLOAT);
        ResourceHandle CreateConstantBuffer(const std::string& name, uint32_t size);
        ResourceHandle CreateStructuredBuffer(const std::string& name, uint32_t elementSize, uint32_t elementCount);

    private:
        friend class RenderGraph;

        RenderGraph* graph;
        
        // 現在構築中のパス情報
        bool inPassContext = false;
        std::string currentPassName;
        std::unordered_map<std::string, ResourceHandle> currentInputs;
        std::unordered_map<std::string, ResourceHandle> currentOutputs;
        std::unordered_map<std::string, float> currentFloatParams;
        std::unordered_map<std::string, int> currentIntParams;
        std::unordered_map<std::string, bool> currentBoolParams;

        // リソース追跡
        std::unordered_map<std::string, ResourceHandle> namedResources;
        std::vector<std::pair<ResourceHandle, std::string>> readDependencies;   // (resource, pass)
        std::vector<std::pair<ResourceHandle, std::string>> writeDependencies;  // (resource, pass)

        /**
         * @brief パスコンテキストを開始
         */
        void BeginPass(const std::string& passName);

        /**
         * @brief パスコンテキストを終了
         */
        void EndPass();

        /**
         * @brief 現在のパス情報をクリア
         */
        void ClearCurrentPass();
    };

    /**
     * @brief パス追加のヘルパーマクロ
     * 
     * 使用例:
     * ATHENA_ADD_PASS(graph, "GeometryPass", GeometryPass)
     *   .AddInput("SceneData", sceneBuffer)
     *   .AddOutput("GBuffer", gBuffer)
     *   .SetFloat("exposure", 1.0f);
     */
    #define ATHENA_ADD_PASS(builder, passName, passType, ...) \
        AddPassHelper<passType>(builder, passName, ##__VA_ARGS__)

    /**
     * @brief パス追加ヘルパー関数
     */
    template<typename PassType, typename... Args>
    RenderGraphBuilder& AddPassHelper(RenderGraphBuilder& builder, const std::string& passName, Args&&... args) {
        // パスを作成
        auto pass = std::make_unique<PassType>(std::forward<Args>(args)...);
        
        // RenderGraphに追加
        builder.graph->AddPass(std::move(pass));
        
        // パスコンテキストを開始
        builder.BeginPass(passName);
        
        return builder;
    }

} // namespace Athena