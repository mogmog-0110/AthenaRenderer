// Mesh.h - メッシュクラス
#pragma once

#include "Athena/Utils/Math.h"
#include "Athena/Resources/Buffer.h"
#include <vector>
#include <memory>

namespace Athena {

    // =====================================================
    // 頂点フォーマット
    // =====================================================

    /**
     * @brief 標準的な頂点構造
     *
     * - Position: 頂点の3D座標（x, y, z）
     * - Normal: 法線ベクトル - 面の向きを示す（ライティング計算に使う）
     * - TexCoord: テクスチャ座標（UV座標）- 画像のどの部分を貼るか
     */
    struct StandardVertex {
        Vector3 position;   // 位置（12バイト）
        Vector3 normal;     // 法線（12バイト）
        float u, v;         // テクスチャ座標（8バイト）

        StandardVertex()
            : position(0, 0, 0)
            , normal(0, 1, 0)
            , u(0), v(0)
        {
        }

        StandardVertex(const Vector3& pos, const Vector3& norm, float u, float v)
            : position(pos)
            , normal(norm)
            , u(u), v(v)
        {
        }

        // 簡易版（法線は後で計算する用）
        StandardVertex(const Vector3& pos, float u, float v)
            : position(pos)
            , normal(0, 1, 0)
            , u(u), v(v)
        {
        }
    };

    // =====================================================
    // サブメッシュ
    // =====================================================

    /**
     * @brief サブメッシュ
     *
     * サブメッシュ = メッシュの一部分
     * 例：車のモデル
     *   - サブメッシュ0: ボディ（赤いマテリアル）
     *   - サブメッシュ1: タイヤ（黒いマテリアル）
     *
     * 異なるマテリアルを使いたい部分ごとに分割する
     */
    struct SubMesh {
        uint32_t indexStart;   // インデックスバッファの開始位置
        uint32_t indexCount;   // 使用するインデックス数
        uint32_t materialID;   // どのマテリアルを使うか（後で実装）

        SubMesh()
            : indexStart(0)
            , indexCount(0)
            , materialID(0)
        {
        }

        SubMesh(uint32_t start, uint32_t count, uint32_t matID = 0)
            : indexStart(start)
            , indexCount(count)
            , materialID(matID)
        {
        }
    };

    // =====================================================
    // メッシュクラス
    // =====================================================

    /**
     * @brief メッシュクラス
     *
     * 3Dモデルの形状データを管理する
     * - 頂点データ
     * - インデックスデータ
     * - サブメッシュ情報
     */
    class Mesh {
    public:
        Mesh();
        ~Mesh();

        // ===== 初期化 =====

        /**
         * @brief CPUメモリにデータを設定
         *
         * @param vertices 頂点データの配列
         * @param vertexCount 頂点数
         * @param indices インデックスデータの配列
         * @param indexCount インデックス数
         */
        void SetVertices(const StandardVertex* vertices, uint32_t vertexCount);
        void SetIndices(const uint32_t* indices, uint32_t indexCount);

        /**
         * @brief GPUバッファを作成してデータをアップロード
         */
        void UploadToGPU(ID3D12Device* device);

        /**
         * @brief サブメッシュを追加
         */
        void AddSubMesh(const SubMesh& subMesh);

        // ===== 描画 =====

        /**
         * @brief 描画コマンドを発行
         *
         * @param commandList 描画コマンドを記録するリスト
         * @param subMeshIndex どのサブメッシュを描画するか（-1 = 全部）
         */
        void Draw(ID3D12GraphicsCommandList* commandList, int subMeshIndex = -1);

        // ===== 情報取得 =====

        uint32_t GetVertexCount() const { return static_cast<uint32_t>(vertices.size()); }
        uint32_t GetIndexCount() const { return static_cast<uint32_t>(indices.size()); }
        uint32_t GetSubMeshCount() const { return static_cast<uint32_t>(subMeshes.size()); }

        const std::vector<StandardVertex>& GetVertices() const { return vertices; }
        const std::vector<uint32_t>& GetIndices() const { return indices; }

        // ===== バッファアクセス =====
        Buffer* GetVertexBuffer() { return vertexBuffer.get(); }
        Buffer* GetIndexBuffer() { return indexBuffer.get(); }

        // ===== ユーティリティ =====

        /**
         * @brief 法線ベクトルを自動計算
         *
         * 法線 = 面の向きを示すベクトル
         * ライティング（光の計算）に必須
         * 三角形の2辺の外積で計算できる
         */
        void CalculateNormals();

        /**
         * @brief バウンディングボックスを計算
         *
         * バウンディングボックス = メッシュを囲む直方体
         * 衝突判定やカリング（画面外判定）に使う
         */
        void CalculateBounds();

        Vector3 GetBoundsMin() const { return boundsMin; }
        Vector3 GetBoundsMax() const { return boundsMax; }
        Vector3 GetBoundsCenter() const { return (boundsMin + boundsMax) * 0.5f; }
        Vector3 GetBoundsSize() const { return boundsMax - boundsMin; }

    private:
        // CPUメモリのデータ
        std::vector<StandardVertex> vertices;
        std::vector<uint32_t> indices;
        std::vector<SubMesh> subMeshes;

        // GPUバッファ
        std::unique_ptr<Buffer> vertexBuffer;
        std::unique_ptr<Buffer> indexBuffer;

        // バウンディングボックス
        Vector3 boundsMin;
        Vector3 boundsMax;

        // 頂点レイアウト（頂点の構造をGPUに教える）
        static constexpr uint32_t VERTEX_STRIDE = sizeof(StandardVertex);  // 32バイト
    };

    // =====================================================
    // プリミティブ生成（便利関数）
    // =====================================================

    /**
     * @brief 基本的な形状を生成するヘルパー関数
     */
    namespace MeshGenerator {

        /**
         * @brief キューブ（立方体）を生成
         */
        std::unique_ptr<Mesh> CreateCube(float size = 1.0f);

        /**
         * @brief 球体を生成
         *
         * @param radius 半径
         * @param slices 経度方向の分割数（横）
         * @param stacks 緯度方向の分割数（縦）
         */
        std::unique_ptr<Mesh> CreateSphere(float radius = 1.0f, uint32_t slices = 32, uint32_t stacks = 16);

        /**
         * @brief 平面を生成
         */
        std::unique_ptr<Mesh> CreatePlane(float width = 1.0f, float depth = 1.0f, uint32_t subdivisions = 1);

    } // namespace MeshGenerator

} // namespace Athena