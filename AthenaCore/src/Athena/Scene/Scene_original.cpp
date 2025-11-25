#include "Athena/Scene/Scene.h"
#include "Athena/Scene/ModelLoader.h"
#include "Athena/Core/Device.h"
#include "Athena/Utils/Logger.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <float.h>

namespace Athena {

    Scene::Scene(const std::string& name) : name(name) {
        Logger::Info("Scene '{}' created", name);
    }

    void Scene::AddObject(std::shared_ptr<SceneObject> object) {
        if (!object) return;
        
        this->objects.push_back(object);
        this->objectsByName[object->GetName()] = object;
        
        Logger::Info("Object '{}' added to scene '{}'", object->GetName(), name);
    }

    void Scene::AddModel(std::shared_ptr<ModelObject> model) {
        if (!model) return;
        
        this->models.push_back(model);
        
        // ModelObjectからSceneObjectを生成してシーンに追加
        auto sceneObjects = model->CreateSceneObjects();
        for (auto& obj : sceneObjects) {
            AddObject(obj);
        }
        
        Logger::Info("Model '{}' added to scene '{}' ({} meshes)", 
                     model->GetName(), name, sceneObjects.size());
    }

    void Scene::RemoveObject(const std::string& name) {
        auto it = this->objectsByName.find(name);
        if (it == this->objectsByName.end()) {
            Logger::Warning("Object '{}' not found in scene '{}'", name, this->name);
            return;
        }
        
        auto objectPtr = it->second;
        this->objectsByName.erase(it);
        
        auto objIt = std::find(this->objects.begin(), this->objects.end(), objectPtr);
        if (objIt != this->objects.end()) {
            this->objects.erase(objIt);
        }
        
        Logger::Info("Object '{}' removed from scene '{}'", name, this->name);
    }

    void Scene::RemoveObject(std::shared_ptr<SceneObject> object) {
        if (!object) return;
        RemoveObject(object->GetName());
    }

    std::shared_ptr<SceneObject> Scene::FindObject(const std::string& name) const {
        auto it = this->objectsByName.find(name);
        return (it != this->objectsByName.end()) ? it->second : nullptr;
    }

    void Scene::ClearObjects() {
        this->objects.clear();
        this->objectsByName.clear();
        this->models.clear();
        Logger::Info("All objects cleared from scene '{}'", name);
    }

    void Scene::AddLight(const std::string& name, const SceneLight& light) {
        this->lights[name] = light;
        Logger::Info("Light '{}' added to scene '{}'", name, this->name);
    }

    void Scene::RemoveLight(const std::string& name) {
        auto it = this->lights.find(name);
        if (it != this->lights.end()) {
            this->lights.erase(it);
            Logger::Info("Light '{}' removed from scene '{}'", name, this->name);
        } else {
            Logger::Warning("Light '{}' not found in scene '{}'", name, this->name);
        }
    }

    SceneLight* Scene::GetLight(const std::string& name) {
        auto it = this->lights.find(name);
        return (it != this->lights.end()) ? &it->second : nullptr;
    }

    const SceneLight* Scene::GetLight(const std::string& name) const {
        auto it = this->lights.find(name);
        return (it != this->lights.end()) ? &it->second : nullptr;
    }

    std::vector<SceneLight> Scene::GetActiveLights() const {
        std::vector<SceneLight> activeLights;
        for (const auto& pair : this->lights) {
            if (pair.second.enabled) {
                activeLights.push_back(pair.second);
            }
        }
        return activeLights;
    }

    void Scene::ClearLights() {
        this->lights.clear();
        Logger::Info("All lights cleared from scene '{}'", name);
    }

    void Scene::Update(float deltaTime) {
        // カメラの更新
        if (this->mainCamera) {
            this->mainCamera->Update(deltaTime);
        }
        
        // 統計情報をリセット
        this->renderStats.Reset();
        this->renderStats.totalObjects = static_cast<uint32_t>(this->objects.size());
    }

    std::vector<std::shared_ptr<SceneObject>> Scene::GetVisibleObjects(const Camera* camera) const {
        // TODO: Implement when compilation issues are resolved
        return std::vector<std::shared_ptr<SceneObject>>();
    }

    std::vector<std::shared_ptr<SceneObject>> Scene::GetTransparentObjects(const Camera* camera) const {
        // TODO: Implement when compilation issues are resolved
        return std::vector<std::shared_ptr<SceneObject>>();
    }

    void Scene::CalculateSceneBounds(Vector3& outMin, Vector3& outMax) const {
        // TODO: Implement when compilation issues are resolved
        outMin = outMax = Vector3(0, 0, 0);
    }

    std::shared_ptr<SceneObject> Scene::FindNearestObject(const Vector3& position) const {
        // TODO: Implement when compilation issues are resolved
        return std::shared_ptr<SceneObject>();
    }

    std::vector<std::shared_ptr<SceneObject>> Scene::RayIntersect(const Vector3& origin, const Vector3& direction) const {
        // TODO: Implement when compilation issues are resolved
        return std::vector<std::shared_ptr<SceneObject>>();
    }

    void Scene::FrameAll(float margin) {
        // TODO: Implement when compilation issues are resolved
        Logger::Info("FrameAll not implemented yet for scene '{}'", name);
    }

    void Scene::FrameSelected(const std::vector<std::string>& objectNames, float margin) {
        // TODO: Implement when compilation issues are resolved
        Logger::Info("FrameSelected not implemented yet for scene '{}'", name);
    }

    void Scene::CreateDefaultLighting() {
        // デフォルト方向ライト
        SceneLight sunLight(SceneLight::Directional);
        sunLight.direction = Vector3(-0.5f, -1.0f, -0.3f).Normalize();
        sunLight.color = Vector3(1.0f, 0.95f, 0.8f);
        sunLight.intensity = 3.0f;
        AddLight("Sun", sunLight);
        
        // 環境光の設定
        this->environment.ambientColor = Vector3(0.3f, 0.4f, 0.6f);
        this->environment.ambientIntensity = 0.2f;
        
        Logger::Info("Default lighting created for scene '{}'", name);
    }

    std::shared_ptr<ModelObject> Scene::LoadAndAddModel(
        std::shared_ptr<Device> device,
        const std::string& filepath,
        const std::string& name,
        const Vector3& position) {
        
        // モデルを読み込み
        auto model = ModelLoader::LoadModel(device, filepath);
        if (!model) {
            Logger::Error("Failed to load model: {}", filepath);
            return nullptr;
        }
        
        // ModelObjectを作成
        std::string modelName = name.empty() ? model->filename : name;
        auto modelObject = std::make_shared<ModelObject>(modelName);
        modelObject->SetModel(model);
        modelObject->GetTransform().position = position;
        
        // シーンに追加
        AddModel(modelObject);
        
        Logger::Info("Model '{}' loaded and added to scene '{}'", modelName, this->name);
        
        return modelObject;
    }

    bool Scene::IsObjectInFrustum(const SceneObject* object, const Camera* camera) const {
        // TODO: Implement when frustum culling is needed
        return true; // Always visible for now
    }

    void Scene::CalculateFrustumPlanes(const Camera* camera, Vector4 frustumPlanes[6]) const {
        // TODO: Implement when Matrix4x4 access is fixed
        for (int i = 0; i < 6; ++i) {
            frustumPlanes[i] = Vector4(0, 0, 0, 1);
        }
    }

    bool Scene::AABBIntersectsFrustum(const Vector3& aabbMin, const Vector3& aabbMax, const Vector4 frustumPlanes[6]) const {
        // TODO: Implement when frustum culling is needed
        return true; // Always visible for now
    }

} // namespace Athena