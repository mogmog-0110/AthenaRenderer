#pragma once
#include "ResourceHandle.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <functional>
#include <d3d12.h>

namespace Athena {

    // Forward declarations
    class RenderContext;
    class RenderGraphBuilder;

    /**
     * @brief レンダーパスの実行データ
     * 
     * 各レンダーパスの実行時に渡される情報を格納
     */
    struct PassExecuteData {
        // 入力リソース（パス名 → ResourceHandle）
        std::unordered_map<std::string, ResourceHandle> inputs;
        
        // 出力リソース（パス名 → ResourceHandle） 
        std::unordered_map<std::string, ResourceHandle> outputs;
        
        // レンダリングコンテキスト
        RenderContext* renderContext = nullptr;
        
        // D3D12関連
        ID3D12GraphicsCommandList* commandList = nullptr;
        ID3D12DescriptorHeap* srvHeap = nullptr;
        
        // 実行時パラメータ（パス固有の設定値など）
        std::unordered_map<std::string, float> floatParams;
        std::unordered_map<std::string, int> intParams;
        std::unordered_map<std::string, bool> boolParams;
        
        /**
         * @brief 入力リソースを取得
         */
        ResourceHandle GetInput(const std::string& name) const {
            auto it = inputs.find(name);
            return (it != inputs.end()) ? it->second : ResourceHandle{};
        }
        
        /**
         * @brief 出力リソースを取得
         */
        ResourceHandle GetOutput(const std::string& name) const {
            auto it = outputs.find(name);
            return (it != outputs.end()) ? it->second : ResourceHandle{};
        }
        
        /**
         * @brief パラメータを取得
         */
        float GetFloat(const std::string& name, float defaultValue = 0.0f) const {
            auto it = floatParams.find(name);
            return (it != floatParams.end()) ? it->second : defaultValue;
        }
        
        int GetInt(const std::string& name, int defaultValue = 0) const {
            auto it = intParams.find(name);
            return (it != intParams.end()) ? it->second : defaultValue;
        }
        
        bool GetBool(const std::string& name, bool defaultValue = false) const {
            auto it = boolParams.find(name);
            return (it != boolParams.end()) ? it->second : defaultValue;
        }
    };

    /**
     * @brief レンダーパスの設定情報
     */
    struct PassSetupData {
        RenderGraphBuilder* builder = nullptr;
        ID3D12Device* device = nullptr; // D3D12デバイス
        
        // パス固有の設定パラメータ
        std::unordered_map<std::string, float> floatParams;
        std::unordered_map<std::string, int> intParams;
        std::unordered_map<std::string, bool> boolParams;
        
        /**
         * @brief パラメータを設定
         */
        void SetFloat(const std::string& name, float value) {
            floatParams[name] = value;
        }
        
        void SetInt(const std::string& name, int value) {
            intParams[name] = value;
        }
        
        void SetBool(const std::string& name, bool value) {
            boolParams[name] = value;
        }
    };

    /**
     * @brief 全レンダーパスの基底クラス
     * 
     * RenderPassは以下の責任を持つ：
     * 1. Setup(): 必要なリソースの宣言
     * 2. Execute(): 実際の描画処理
     * 3. リソースの入出力定義
     */
    class RenderPass {
    public:
        /**
         * @brief コンストラクタ
         * @param passName パス名（デバッグ・識別用）
         */
        explicit RenderPass(const std::string& passName) : name(passName) {}
        
        virtual ~RenderPass() = default;

        /**
         * @brief パス名を取得
         */
        const std::string& GetName() const { return name; }

        /**
         * @brief パスのセットアップ（リソース宣言）
         * 
         * この関数で：
         * - 必要な入力リソースを宣言
         * - 生成する出力リソースを宣言
         * - リソース間の依存関係を定義
         * 
         * @param setupData セットアップ用データ（RenderGraphBuilderへのアクセス等）
         */
        virtual void Setup(PassSetupData& setupData) = 0;

        /**
         * @brief パスの実行（実際の描画処理）
         * 
         * この関数で：
         * - SetupDataで宣言したリソースを使用
         * - 実際のGPUコマンドを発行
         * - 描画処理を実行
         * 
         * @param executeData 実行用データ（リソースハンドル、レンダリングコンテキスト等）
         */
        virtual void Execute(const PassExecuteData& executeData) = 0;

        /**
         * @brief パスが有効かチェック
         * 
         * 条件に応じてパスをスキップしたい場合にオーバーライド
         * 
         * @param setupData セットアップデータ
         * @return パスを実行する場合true、スキップする場合false
         */
        virtual bool IsEnabled(const PassSetupData& setupData) const { 
            return true; 
        }

        /**
         * @brief パスの説明を取得（デバッグ・UI表示用）
         */
        virtual std::string GetDescription() const { 
            return "RenderPass: " + name; 
        }

        /**
         * @brief パスのカテゴリを取得
         */
        virtual std::string GetCategory() const { 
            return "General"; 
        }

    protected:
        std::string name;
    };

    /**
     * @brief レンダーパスファクトリー関数の型定義
     */
    using PassFactoryFunction = std::function<std::unique_ptr<RenderPass>()>;

    /**
     * @brief レンダーパス登録情報
     */
    struct PassRegistryEntry {
        std::string name;           // パス名
        std::string category;       // カテゴリ
        std::string description;    // 説明
        PassFactoryFunction factory; // ファクトリー関数
    };

} // namespace Athena