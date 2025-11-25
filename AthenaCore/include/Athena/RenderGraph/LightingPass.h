#pragma once

#include "RenderPass.h"
#include "../Resources/Buffer.h"
#include "../Resources/Texture.h"
#include "../Utils/Math.h"
#include <d3d12.h>
#include <wrl/client.h>
#include <memory>

namespace Athena {

    using Microsoft::WRL::ComPtr;

    /**
     * @brief ライティングパス - ディファードライティング
     * 
     * G-Buffer（ジオメトリバッファ）から読み取った情報を使用して
     * ディファードライティングを実行するパスである。
     * 複数のライトを効率的に処理できる。
     */
    class LightingPass : public RenderPass {
    public:
        /**
         * @brief ライト情報構造体
         */
        struct LightData {
            Vector3 position;       // ライト位置（ポイントライト用）
            float intensity;        // ライト強度
            Vector3 direction;      // ライト方向（方向ライト用）
            float range;           // ライト範囲
            Vector3 color;         // ライト色
            int type;              // ライトタイプ（0:方向ライト, 1:ポイントライト, 2:スポットライト）
        };

        /**
         * @brief ライティング定数バッファ
         */
        struct LightingConstants {
            Vector3 cameraPosition;     // カメラ位置
            float exposure;             // 露出値
            Vector3 ambientColor;       // 環境光色
            uint32_t lightCount;        // ライト数
            Matrix4x4 invViewMatrix;    // 逆ビュー行列（ワールド座標復元用）
            Matrix4x4 invProjMatrix;    // 逆プロジェクション行列（ワールド座標復元用）
            LightData lights[8];        // 最大8個のライト
            // 256バイトアライメント用パディング（行列追加で調整）
            float paddingArray[8];
        };

        LightingPass() : RenderPass("LightingPass") {
            cameraPosition = Vector3(0.0f, 0.0f, 0.0f);
            ambientColor = Vector3(0.1f, 0.1f, 0.1f);
        }
        virtual ~LightingPass();

        /**
         * @brief パスのセットアップ
         */
        void Setup(PassSetupData& setupData) override;

        /**
         * @brief パスの実行
         */
        void Execute(const PassExecuteData& executeData) override;

        /**
         * @brief G-Bufferテクスチャを設定
         */
        void SetGBuffer(std::shared_ptr<Texture> albedo,
                       std::shared_ptr<Texture> normal,
                       std::shared_ptr<Texture> depth) {
            gbufferAlbedo = albedo;
            gbufferNormal = normal;
            gbufferDepth = depth;
        }

        /**
         * @brief カメラ情報を設定
         */
        void SetCamera(const Vector3& position, float exposure = 1.0f) {
            cameraPosition = position;
            this->exposure = exposure;
        }

        /**
         * @brief 変換行列を設定（座標復元用）
         */
        void SetTransform(const Matrix4x4& view, const Matrix4x4& proj) {
            invViewMatrix = view.Inverse();
            invProjMatrix = proj.Inverse();
        }

        /**
         * @brief 環境光を設定
         */
        void SetAmbientLight(const Vector3& color) {
            ambientColor = color;
        }

        /**
         * @brief 方向ライトを追加
         */
        void AddDirectionalLight(const Vector3& direction, const Vector3& color, float intensity = 1.0f);

        /**
         * @brief ポイントライトを追加
         */
        void AddPointLight(const Vector3& position, const Vector3& color, float range, float intensity = 1.0f);

        /**
         * @brief 全ライトをクリア
         */
        void ClearLights() { lightCount = 0; }

        /**
         * @brief レンダーターゲットサイズを設定
         */
        void SetRenderTargetSize(uint32_t width, uint32_t height) {
            renderTargetWidth = width;
            renderTargetHeight = height;
        }

    private:
        // パイプライン状態
        ComPtr<ID3D12RootSignature> rootSignature;
        ComPtr<ID3D12PipelineState> pipelineState;

        // バッファ
        std::unique_ptr<Buffer> constantBuffer;

        // G-Bufferテクスチャ
        std::shared_ptr<Texture> gbufferAlbedo;  // アルベド + メタリック
        std::shared_ptr<Texture> gbufferNormal;  // 法線 + ラフネス
        std::shared_ptr<Texture> gbufferDepth;   // 深度

        // ライティングデータ
        LightingConstants constants = {};
        Vector3 cameraPosition;
        Vector3 ambientColor;
        float exposure = 1.0f;
        uint32_t lightCount = 0;
        LightData lights[8] = {};
        Matrix4x4 invViewMatrix;
        Matrix4x4 invProjMatrix;

        // レンダーターゲットサイズ
        uint32_t renderTargetWidth = 1280;
        uint32_t renderTargetHeight = 720;

        /**
         * @brief パイプライン状態を作成
         */
        void CreatePipelineState(ID3D12Device* device);

        /**
         * @brief ルートシグネチャを作成
         */
        void CreateRootSignature(ID3D12Device* device);

        /**
         * @brief シェーダーをコンパイル
         */
        ComPtr<ID3DBlob> CompileShader(const std::wstring& filepath, 
                                      const std::string& entryPoint, 
                                      const std::string& target);

        /**
         * @brief 定数バッファを更新
         */
        void UpdateConstants();
    };

} // namespace Athena