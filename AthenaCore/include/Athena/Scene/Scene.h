#pragma once

#include "SceneObject.h"
#include "Camera.h"
#include "CameraController.h"
#include "../Utils/Math.h"
#include <memory>
#include <vector>
#include <unordered_map>
#include <string>
#include <functional>

namespace Athena {

    class Device;

    /**
     * @brief ライト情報（シーン用）
     */
    struct SceneLight {
        enum Type {
            Directional = 0,
            Point = 1,
            Spot = 2
        };
        
        Type type = Directional;
        Vector3 position = Vector3(0.0f, 10.0f, 0.0f);
        Vector3 direction = Vector3(0.0f, -1.0f, 0.0f);
        Vector3 color = Vector3(1.0f, 1.0f, 1.0f);
        float intensity = 1.0f;
        float range = 50.0f;      // ポイントライト・スポットライト用
        float innerCone = 30.0f;  // スポットライト用（度）
        float outerCone = 45.0f;  // スポットライト用（度）
        bool enabled = true;
        
        SceneLight(Type t = Directional) : type(t) {}
    };

    /**
     * @brief シーンの環境設定
     */
    struct SceneEnvironment {
        Vector3 ambientColor = Vector3(0.1f, 0.1f, 0.1f);
        float ambientIntensity = 1.0f;
        Vector3 backgroundColor = Vector3(0.2f, 0.3f, 0.6f);
        float exposure = 1.0f;
        float gamma = 2.2f;
        bool enableToneMapping = true;
        bool enableGammaCorrection = true;
    };

    /**
     * @brief レンダリング統計情報
     */
    struct RenderStats {
        uint32_t totalObjects = 0;
        uint32_t renderedObjects = 0;
        uint32_t culledObjects = 0;
        uint32_t totalTriangles = 0;
        uint32_t totalVertices = 0;
        uint32_t drawCalls = 0;
        float frameTime = 0.0f;
        float cullingTime = 0.0f;
        float renderingTime = 0.0f;
        
        void Reset() {
            totalObjects = renderedObjects = culledObjects = 0;
            totalTriangles = totalVertices = drawCalls = 0;
            frameTime = cullingTime = renderingTime = 0.0f;
        }
    };

    /**
     * @brief シーン管理クラス
     * 
     * 3Dシーン内のオブジェクト、ライト、カメラ、環境設定を管理し、
     * フラスタムカリング、ソート、バッチング等の最適化を提供する。
     */
    class Scene {
    public:
        Scene(const std::string& name = "Scene");
        ~Scene() = default;

        // ===== 基本情報 =====
        const std::string& GetName() const { return name; }
        void SetName(const std::string& name) { this->name = name; }

        // ===== オブジェクト管理 =====
        
        /**
         * @brief SceneObjectを追加
         */
        void AddObject(std::shared_ptr<SceneObject> object);
        
        /**
         * @brief ModelObjectを追加（自動的にSceneObjectに展開）
         */
        void AddModel(std::shared_ptr<ModelObject> model);
        
        /**
         * @brief オブジェクトを削除
         */
        void RemoveObject(const std::string& name);
        void RemoveObject(std::shared_ptr<SceneObject> object);
        
        /**
         * @brief オブジェクトを検索
         */
        std::shared_ptr<SceneObject> FindObject(const std::string& name) const;
        
        /**
         * @brief 全オブジェクトを取得
         */
        std::vector<std::shared_ptr<SceneObject>> GetObjects() const;
        
        /**
         * @brief オブジェクトをクリア
         */
        void ClearObjects();

        // ===== ライト管理 =====
        
        /**
         * @brief ライトを追加
         */
        void AddLight(const std::string& name, const SceneLight& light);
        
        /**
         * @brief ライトを削除
         */
        void RemoveLight(const std::string& name);
        
        /**
         * @brief ライトを取得
         */
        SceneLight* GetLight(const std::string& name);
        const SceneLight* GetLight(const std::string& name) const;
        
        /**
         * @brief 全ライトを取得
         */
        std::vector<SceneLight> GetActiveLights() const;
        
        /**
         * @brief ライトをクリア
         */
        void ClearLights();

        // ===== カメラ管理 =====
        
        /**
         * @brief メインカメラを設定
         */
        void SetMainCamera(std::shared_ptr<CameraController> camera) { mainCamera = camera; }
        std::shared_ptr<CameraController> GetMainCamera() const { return mainCamera; }

        // ===== 環境設定 =====
        
        SceneEnvironment& GetEnvironment();
        const SceneEnvironment& GetEnvironment() const;

        // ===== 更新・レンダリング =====
        
        /**
         * @brief シーンの更新
         */
        void Update(float deltaTime);
        
        /**
         * @brief 可視オブジェクトを取得（フラスタムカリング済み）
         */
        std::vector<std::shared_ptr<SceneObject>> GetVisibleObjects(const Camera* camera) const;
        
        /**
         * @brief 距離でソートされた透明オブジェクトを取得
         */
        std::vector<std::shared_ptr<SceneObject>> GetTransparentObjects(const Camera* camera) const;
        
        /**
         * @brief シーンをレンダリング
         */
        void Render(std::shared_ptr<Camera> camera);
        
        /**
         * @brief レンダリング統計を取得
         */
        RenderStats GetRenderStats() const;
        void ResetRenderStats();

        // ===== ユーティリティ =====
        
        /**
         * @brief シーン全体のバウンディングボックスを計算
         */
        void CalculateSceneBounds(Vector3& outMin, Vector3& outMax) const;
        
        /**
         * @brief 指定位置に最も近いオブジェクトを検索
         */
        std::shared_ptr<SceneObject> FindNearestObject(const Vector3& position) const;
        
        /**
         * @brief レイとの交差判定
         */
        std::vector<std::shared_ptr<SceneObject>> RayIntersect(const Vector3& origin, const Vector3& direction) const;
        
        /**
         * @brief カメラに全オブジェクトが映るようにフレーミング
         */
        void FrameAll(float margin = 1.2f);
        
        /**
         * @brief 選択されたオブジェクトにフレーミング
         */
        void FrameSelected(const std::vector<std::string>& objectNames, float margin = 1.2f);

        // ===== デバッグ・可視化 =====
        
        /**
         * @brief ワイヤーフレーム表示を切り替え
         */
        void SetWireframeMode(bool enable);
        bool GetWireframeMode() const;
        
        /**
         * @brief バウンディングボックス表示を切り替え
         */
        void SetShowBounds(bool show);
        bool GetShowBounds() const;
        
        /**
         * @brief ライト可視化を切り替え
         */
        void SetShowLights(bool show);
        bool GetShowLights() const;

        // ===== 便利な作成関数 =====
        
        /**
         * @brief デフォルトライトを作成
         */
        void CreateDefaultLighting();
        
        /**
         * @brief モデルファイルを読み込んでシーンに追加
         */
        std::shared_ptr<ModelObject> LoadAndAddModel(
            std::shared_ptr<Device> device,
            const std::string& filepath,
            const std::string& name = "",
            const Vector3& position = Vector3(0, 0, 0)
        );

    private:
        std::string name;
        
        // オブジェクト管理
        std::vector<std::shared_ptr<SceneObject>> objects;
        std::unordered_map<std::string, std::shared_ptr<SceneObject>> objectsByName;
        
        // モデルオブジェクト管理（生成したSceneObjectの元となるModelObjectを保持）
        std::vector<std::shared_ptr<ModelObject>> models;
        
        // ライト管理
        std::unordered_map<std::string, SceneLight> lights;
        
        // カメラ
        std::shared_ptr<CameraController> mainCamera;
        
        // 環境設定
        SceneEnvironment environment;
        
        // デバッグ設定
        bool wireframeMode = false;
        bool showBounds = false;
        bool showLights = false;
        
        // 統計情報
        mutable RenderStats renderStats;
        
        /**
         * @brief フラスタムカリング
         */
        bool IsObjectInFrustum(const SceneObject* object, const Camera* camera) const;
        
        /**
         * @brief フラスタムプレーンを計算
         */
        void CalculateFrustumPlanes(const Camera* camera, Vector4 frustumPlanes[6]) const;
        
        /**
         * @brief AABBとフラスタムの交差判定
         */
        bool AABBIntersectsFrustum(const Vector3& aabbMin, const Vector3& aabbMax, const Vector4 frustumPlanes[6]) const;
    };

} // namespace Athena