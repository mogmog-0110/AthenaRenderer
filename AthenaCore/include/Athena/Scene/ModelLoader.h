#pragma once

#include "../Resources/Buffer.h"
#include "../Resources/Texture.h" 
#include "../Utils/Math.h"
#include "Mesh.h"
#include <memory>
#include <vector>
#include <string>
#include <unordered_map>

// Forward declarations for Assimp
struct aiScene;
struct aiMesh;
struct aiNode;
struct aiMaterial;
enum aiTextureType;

namespace Athena {

    class Device;

    /**
     * @brief モデル読み込み用の頂点データ
     */
    struct ModelVertex {
        Vector3 position;
        Vector3 normal;
        Vector2 texcoord;
        Vector3 tangent;    // 法線マッピング用
        Vector3 bitangent;  // 法線マッピング用
    };

    /**
     * @brief マテリアル情報
     */
    struct MaterialData {
        Vector3 albedo = Vector3(1.0f, 1.0f, 1.0f);
        float metallic = 0.0f;
        float roughness = 1.0f;
        float ao = 1.0f;
        std::string name;
        
        // テクスチャパス
        std::string albedoTexturePath;
        std::string normalTexturePath;
        std::string metallicTexturePath;
        std::string roughnessTexturePath;
        std::string aoTexturePath;
        
        // 読み込まれたテクスチャ
        std::shared_ptr<Texture> albedoTexture;
        std::shared_ptr<Texture> normalTexture;
        std::shared_ptr<Texture> metallicTexture;
        std::shared_ptr<Texture> roughnessTexture;
        std::shared_ptr<Texture> aoTexture;
    };

    /**
     * @brief 読み込まれたモデルデータ
     */
    struct Model {
        std::vector<std::shared_ptr<Mesh>> meshes;
        std::vector<MaterialData> materials;
        std::string directory;     // モデルファイルのディレクトリ
        std::string filename;      // モデルファイル名
        
        // バウンディングボックス
        Vector3 minBounds = Vector3(FLT_MAX, FLT_MAX, FLT_MAX);
        Vector3 maxBounds = Vector3(-FLT_MAX, -FLT_MAX, -FLT_MAX);
        
        /**
         * @brief モデル全体のバウンディングボックスを計算
         */
        void CalculateBounds();
        
        /**
         * @brief モデルの中心座標を取得
         */
        Vector3 GetCenter() const {
            return (minBounds + maxBounds) * 0.5f;
        }
        
        /**
         * @brief モデルのサイズを取得
         */
        Vector3 GetSize() const {
            return maxBounds - minBounds;
        }
    };

    /**
     * @brief 3Dモデル読み込みクラス
     * 
     * Assimpライブラリを使用してOBJ, FBX, glTFなど様々な形式の
     * 3Dモデルファイルを読み込み、DirectX 12用のリソースに変換する。
     */
    class ModelLoader {
    public:
        ModelLoader() = default;
        ~ModelLoader() = default;

        /**
         * @brief モデルファイルを読み込み
         * @param device DirectX 12デバイス
         * @param filepath モデルファイルパス
         * @param flipUV UVのY軸を反転するか（DirectXではtrueが一般的）
         * @param generateTangents 接線ベクトルを生成するか
         * @return 読み込まれたモデルデータ（失敗時はnullptr）
         */
        static std::shared_ptr<Model> LoadModel(
            std::shared_ptr<Device> device,
            const std::string& filepath,
            bool flipUV = true,
            bool generateTangents = true
        );

        /**
         * @brief サポートされている拡張子一覧を取得
         */
        static std::vector<std::string> GetSupportedExtensions();

        /**
         * @brief 拡張子がサポートされているかチェック
         */
        static bool IsExtensionSupported(const std::string& extension);

    private:
        /**
         * @brief Assimpシーンを処理
         */
        static void ProcessNode(
            const aiScene* scene,
            aiNode* node,
            Model* model,
            std::shared_ptr<Device> device,
            const std::string& directory
        );

        /**
         * @brief Assimpメッシュを処理
         */
        static std::shared_ptr<Mesh> ProcessMesh(
            const aiScene* scene,
            aiMesh* mesh,
            uint32_t materialIndex,
            std::shared_ptr<Device> device
        );

        /**
         * @brief Assimpマテリアルを処理
         */
        static MaterialData ProcessMaterial(
            const aiScene* scene,
            aiMaterial* mat,
            const std::string& directory,
            std::shared_ptr<Device> device
        );

        /**
         * @brief テクスチャを読み込み
         */
        static std::shared_ptr<Texture> LoadTexture(
            const std::string& path,
            const std::string& directory,
            std::shared_ptr<Device> device,
            bool sRGB = false
        );

        /**
         * @brief マテリアルからテクスチャパスを取得
         */
        static std::string GetTexturePathFromMaterial(
            aiMaterial* mat,
            aiTextureType type,
            const std::string& directory
        );

        // Helper functions moved to implementation file to avoid Assimp type exposure

        // テクスチャキャッシュ（同じテクスチャの重複読み込みを防ぐ）
        // static std::unordered_map<std::string, std::shared_ptr<Texture>> textureCache;
    };

} // namespace Athena