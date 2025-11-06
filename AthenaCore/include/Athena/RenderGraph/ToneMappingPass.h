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
     * @brief トーンマッピングパス - HDRからLDRへの変換
     * 
     * HDR（High Dynamic Range）レンダリング結果を
     * ディスプレイ表示可能なLDR（Low Dynamic Range）に変換するパスである。
     * ガンマ補正や露出調整も含む。
     */
    class ToneMappingPass : public RenderPass {
    public:
        /**
         * @brief トーンマッピング手法
         */
        enum class ToneMappingMethod {
            Linear,         // リニア（クランプのみ）
            Reinhard,       // Reinhard演算子
            ReinhardLuma,   // Reinhard輝度ベース
            ACES,           // ACESフィルミック
            Uncharted2,     // Uncharted 2フィルミック
        };

        /**
         * @brief トーンマッピング定数バッファ
         */
        struct ToneMappingConstants {
            float exposure;             // 露出値
            float gamma;                // ガンマ値
            int toneMappingMethod;      // トーンマッピング手法
            float whitePoint;           // ホワイトポイント
            float contrast;             // コントラスト
            float brightness;           // 明度
            float saturation;           // 彩度
            float padding;              // パディング
            // 256バイトアライメント用パディング
            float paddingArray[56];
        };

        ToneMappingPass() : RenderPass("ToneMappingPass") {}
        virtual ~ToneMappingPass();

        /**
         * @brief パスのセットアップ
         */
        void Setup(PassSetupData& setupData) override;

        /**
         * @brief パスの実行
         */
        void Execute(const PassExecuteData& executeData) override;

        /**
         * @brief HDRテクスチャを設定
         */
        void SetHDRTexture(std::shared_ptr<Texture> hdrTexture) {
            this->hdrTexture = hdrTexture;
        }

        /**
         * @brief 露出値を設定
         */
        void SetExposure(float exposure) {
            this->exposure = exposure;
        }

        /**
         * @brief ガンマ値を設定
         */
        void SetGamma(float gamma) {
            this->gamma = gamma;
        }

        /**
         * @brief トーンマッピング手法を設定
         */
        void SetToneMappingMethod(ToneMappingMethod method) {
            toneMappingMethod = method;
        }

        /**
         * @brief ホワイトポイントを設定
         */
        void SetWhitePoint(float whitePoint) {
            this->whitePoint = whitePoint;
        }

        /**
         * @brief 色調補正パラメータを設定
         */
        void SetColorGrading(float contrast, float brightness, float saturation) {
            this->contrast = contrast;
            this->brightness = brightness;
            this->saturation = saturation;
        }

    private:
        // パイプライン状態
        ComPtr<ID3D12RootSignature> rootSignature;
        ComPtr<ID3D12PipelineState> pipelineState;

        // バッファ
        std::unique_ptr<Buffer> constantBuffer;

        // HDRテクスチャ
        std::shared_ptr<Texture> hdrTexture;

        // トーンマッピングパラメータ
        float exposure = 1.0f;
        float gamma = 2.2f;
        ToneMappingMethod toneMappingMethod = ToneMappingMethod::ACES;
        float whitePoint = 11.2f;
        float contrast = 1.0f;
        float brightness = 0.0f;
        float saturation = 1.0f;

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