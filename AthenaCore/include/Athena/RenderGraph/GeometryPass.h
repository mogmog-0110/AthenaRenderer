#pragma once

#include "RenderPass.h"
#include "../Resources/Buffer.h"
#include "../Resources/Texture.h"
#include "../Utils/Math.h"
#include "../Core/DescriptorHeap.h"
#include <d3d12.h>
#include <wrl/client.h>
#include <memory>
#include <vector>
#include <unordered_map>

namespace Athena {

    using Microsoft::WRL::ComPtr;

    /**
     * @brief ジオメトリパス - 3Dオブジェクトの基本レンダリング
     * 
     * 基本的な3Dメッシュのレンダリングを行うパスである。
     * 頂点データとインデックスデータを受け取り、MVP変換を適用して
     * レンダーターゲットに描画する。
     */
    class GeometryPass : public RenderPass {
    public:
        /**
         * @brief 頂点データ構造体
         */
        struct Vertex {
            Vector3 position;
            Vector3 normal;
            Vector2 texcoord; // UV座標をVector2に統一
        };

        /**
         * @brief 定数バッファ構造体（256バイトアライメント）- Bindless対応
         */
        struct GeometryConstants {
            Matrix4x4 mvpMatrix;        // Model-View-Projection行列
            Matrix4x4 worldMatrix;      // ワールド行列
            Matrix4x4 viewMatrix;       // ビュー行列
            Matrix4x4 projMatrix;       // プロジェクション行列
            Vector3 cameraPosition;     // カメラ位置
            float padding1;
            Vector3 lightDirection;     // ライト方向
            float padding2;
            Vector3 lightColor;         // ライト色
            uint32_t textureIndex;      // Bindlessテクスチャインデックス
            uint32_t materialIndex;     // マテリアルインデックス（将来用）
            uint32_t objectID;          // オブジェクトID
            // 追加パディングで256バイトに調整
            float paddingArray[26];
        };

        GeometryPass() : RenderPass("GeometryPass") {
            worldMatrix = Matrix4x4::CreateIdentity();
            viewMatrix = Matrix4x4::CreateIdentity();
            projMatrix = Matrix4x4::CreateIdentity();
            cameraPosition = Vector3(0.0f, 0.0f, 0.0f);
            lightDirection = Vector3(0.0f, -1.0f, 0.0f);
            lightColor = Vector3(1.0f, 1.0f, 1.0f);
        }
        virtual ~GeometryPass();

        /**
         * @brief パスのセットアップ
         */
        void Setup(PassSetupData& setupData) override;

        /**
         * @brief パスの実行
         */
        void Execute(const PassExecuteData& executeData) override;

        /**
         * @brief 頂点バッファを設定
         */
        void SetVertexData(const Vertex* vertices, uint32_t vertexCount, 
                          const uint32_t* indices, uint32_t indexCount);

        /**
         * @brief 変換行列を設定
         */
        void SetTransform(const Matrix4x4& world, const Matrix4x4& view, const Matrix4x4& proj);

        /**
         * @brief カメラ位置を設定
         */
        void SetCameraPosition(const Vector3& position) { cameraPosition = position; }

        /**
         * @brief ライト設定
         */
        void SetLight(const Vector3& direction, const Vector3& color) {
            lightDirection = direction;
            lightColor = color;
        }

        /**
         * @brief テクスチャを設定（従来方式）
         */
        void SetTexture(std::shared_ptr<Texture> texture) { mainTexture = texture; }

        /**
         * @brief Bindless用テクスチャインデックスを設定
         */
        void SetTextureIndex(uint32_t index) { constants.textureIndex = index; }

        /**
         * @brief オブジェクトIDを設定（Multiple描画用）
         */
        void SetObjectID(uint32_t id) { constants.objectID = id; }

        /**
         * @brief Bindlessデスクリプタヒープを初期化
         */
        void InitializeBindlessHeap(ID3D12Device* device, uint32_t maxTextures = 1000);

        /**
         * @brief テクスチャをBindlessヒープに追加
         */
        uint32_t AddTextureToBindlessHeap(std::shared_ptr<Texture> texture, ID3D12Device* device);

    private:
        // パイプライン状態
        ComPtr<ID3D12RootSignature> rootSignature;
        ComPtr<ID3D12PipelineState> pipelineState;

        // バッファ
        std::unique_ptr<Buffer> vertexBuffer;
        std::unique_ptr<Buffer> indexBuffer;
        std::unique_ptr<Buffer> constantBuffer;

        // テクスチャ
        std::shared_ptr<Texture> mainTexture;

        // Bindlessレンダリング用
        std::unique_ptr<DescriptorHeap> bindlessHeap;
        std::unordered_map<std::shared_ptr<Texture>, uint32_t> textureIndexMap;
        uint32_t nextTextureIndex = 0;
        bool bindlessEnabled = false;

        // レンダリングデータ
        uint32_t indexCount = 0;
        uint32_t vertexCount = 0;
        GeometryConstants constants = {};

        // 頂点データのキャッシュ
        std::vector<Vertex> cachedVertices;
        std::vector<uint32_t> cachedIndices;
        bool buffersDirty = false;

        // デバイス参照（バッファ更新用）
        ID3D12Device* device = nullptr;

        // 変換行列
        Matrix4x4 worldMatrix;
        Matrix4x4 viewMatrix;
        Matrix4x4 projMatrix;
        Vector3 cameraPosition;
        Vector3 lightDirection;
        Vector3 lightColor;

        /**
         * @brief パイプライン状態を作成
         */
        void CreatePipelineState(ID3D12Device* device);

        /**
         * @brief ルートシグネチャを作成
         */
        void CreateRootSignature(ID3D12Device* device);

        /**
         * @brief 頂点/インデックスバッファを更新
         */
        void UpdateBuffers(ID3D12Device* device);

        /**
         * @brief シェーダーをコンパイル
         */
        ComPtr<ID3DBlob> CompileShader(const std::wstring& filepath, 
                                      const std::string& entryPoint, 
                                      const std::string& target);
    };

} // namespace Athena