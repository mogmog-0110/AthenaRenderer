#pragma once

#include "Mesh.h"
#include "ModelLoader.h"  // Re-enabled with stub implementation
#include "../Utils/Math.h"
#include <memory>
#include <string>

namespace Athena {

    /**
     * @brief Transform情報
     */
    struct Transform {
        Vector3 position = Vector3(0.0f, 0.0f, 0.0f);
        Vector3 rotation = Vector3(0.0f, 0.0f, 0.0f);  // Euler angles in degrees
        Vector3 scale = Vector3(1.0f, 1.0f, 1.0f);
        
        /**
         * @brief ワールド変換行列を取得
         */
        Matrix4x4 GetWorldMatrix() const;
        
        /**
         * @brief 各軸での回転を適用
         */
        void RotateX(float degrees) { rotation.x += degrees; }
        void RotateY(float degrees) { rotation.y += degrees; }
        void RotateZ(float degrees) { rotation.z += degrees; }
        
        /**
         * @brief 移動
         */
        void Translate(const Vector3& translation) { position = position + translation; }
        
        /**
         * @brief 均等スケール
         */
        void ScaleUniform(float factor) { scale = scale * factor; }
    };

    /**
     * @brief マテリアル定数バッファ用データ
     */
    struct MaterialConstants {
        Vector3 albedo = Vector3(1.0f, 1.0f, 1.0f);
        float metallic = 0.0f;
        float roughness = 1.0f;
        float ao = 1.0f;
        Vector2 uvScale = Vector2(1.0f, 1.0f);
        
        // テクスチャフラグ（0=なし, 1=あり）
        uint32_t hasAlbedoTexture = 0;
        uint32_t hasNormalTexture = 0;
        uint32_t hasMetallicTexture = 0;
        uint32_t hasRoughnessTexture = 0;
        uint32_t hasAOTexture = 0;
        
        float padding[3]; // 256バイト境界アライメント用
    };

    /**
     * @brief シーンオブジェクト（1つのMesh + Transform + Material）
     */
    class SceneObject {
    public:
        SceneObject(const std::string& name = "SceneObject");
        ~SceneObject() = default;

        // ===== 基本情報 =====
        const std::string& GetName() const { return name; }
        void SetName(const std::string& name) { this->name = name; }

        // ===== Transform =====
        Transform& GetTransform() { return transform; }
        const Transform& GetTransform() const { return transform; }
        
        void SetPosition(const Vector3& position) { transform.position = position; }
        void SetRotation(const Vector3& rotation) { transform.rotation = rotation; }
        void SetScale(const Vector3& scale) { transform.scale = scale; }
        
        Vector3 GetPosition() const { return transform.position; }
        Vector3 GetRotation() const { return transform.rotation; }
        Vector3 GetScale() const { return transform.scale; }

        // ===== Mesh =====
        void SetMesh(std::shared_ptr<Mesh> mesh) { this->mesh = mesh; }
        std::shared_ptr<Mesh> GetMesh() const { return mesh; }
        bool HasMesh() const { return mesh != nullptr; }

        // ===== Material =====
        void SetMaterial(const MaterialData& material) { this->material = material; }
        const MaterialData& GetMaterial() const { return material; }
        MaterialData& GetMaterial() { return material; }

        /**
         * @brief マテリアル定数バッファ用データを生成
         */
        MaterialConstants GetMaterialConstants() const;

        // ===== 表示制御 =====
        void SetVisible(bool visible) { this->visible = visible; }
        bool IsVisible() const { return visible; }

        // ===== バウンディング =====
        /**
         * @brief ワールド空間でのバウンディングボックスを取得
         */
        void GetWorldBounds(Vector3& outMin, Vector3& outMax) const;

        /**
         * @brief カメラからの距離を計算（ソート用）
         */
        float CalculateDistanceToCamera(const Vector3& cameraPosition) const;

    private:
        std::string name;
        Transform transform;
        std::shared_ptr<Mesh> mesh;
        MaterialData material;
        bool visible = true;
    };

    /**
     * @brief モデルオブジェクト（複数のMesh + 共通Transform）
     */
    class ModelObject {
    public:
        ModelObject(const std::string& name = "ModelObject");
        ~ModelObject() = default;

        // ===== 基本情報 =====
        const std::string& GetName() const { return name; }
        void SetName(const std::string& name) { this->name = name; }

        // ===== Transform（モデル全体に適用） =====
        Transform& GetTransform() { return transform; }
        const Transform& GetTransform() const { return transform; }

        // ===== Model =====
        void SetModel(std::shared_ptr<Model> model) { this->model = model; }
        std::shared_ptr<Model> GetModel() const { return model; }
        bool HasModel() const { return model != nullptr; }

        // ===== SceneObject生成 =====
        /**
         * @brief Modelの各MeshをSceneObjectとして生成
         */
        std::vector<std::shared_ptr<SceneObject>> CreateSceneObjects() const;

        // ===== 表示制御 =====
        void SetVisible(bool visible) { this->visible = visible; }
        bool IsVisible() const { return visible; }

        // ===== バウンディング =====
        void GetWorldBounds(Vector3& outMin, Vector3& outMax) const;

    private:
        std::string name;
        Transform transform;
        std::shared_ptr<Model> model;
        bool visible = true;
    };

} // namespace Athena