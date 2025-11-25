#include "Athena/Scene/SceneObject.h"
#include "Athena/Utils/Logger.h"
#include <cmath>

namespace Athena {

    Matrix4x4 Transform::GetWorldMatrix() const {
        Matrix4x4 scaleMatrix = Matrix4x4::CreateScale(scale.x, scale.y, scale.z);
        Matrix4x4 rotationX = Matrix4x4::CreateRotationX(rotation.x * PI / 180.0f);
        Matrix4x4 rotationY = Matrix4x4::CreateRotationY(rotation.y * PI / 180.0f);
        Matrix4x4 rotationZ = Matrix4x4::CreateRotationZ(rotation.z * PI / 180.0f);
        Matrix4x4 rotationMatrix = rotationZ * rotationX * rotationY;
        Matrix4x4 translationMatrix = Matrix4x4::CreateTranslation(position.x, position.y, position.z);
        return scaleMatrix * rotationMatrix * translationMatrix;
    }

    SceneObject::SceneObject(const std::string& name) : name(name) {
        material.albedo = Vector3(0.8f, 0.8f, 0.8f);
        material.metallic = 0.0f;
        material.roughness = 0.5f;
        material.ao = 1.0f;
        material.name = "DefaultMaterial";
    }

    MaterialConstants SceneObject::GetMaterialConstants() const {
        MaterialConstants constants;
        constants.albedo = material.albedo;
        constants.metallic = material.metallic;
        constants.roughness = material.roughness;
        constants.ao = material.ao;
        constants.uvScale = Vector2(1.0f, 1.0f);
        // Remove texture flags for now since they're not defined
        return constants;
    }

    void SceneObject::GetWorldBounds(Vector3& outMin, Vector3& outMax) const {
        if (!mesh) {
            outMin = outMax = transform.position;
            return;
        }
        
        Vector3 meshMin = mesh->GetBoundsMin();
        Vector3 meshMax = mesh->GetBoundsMax();
        
        Vector3 corners[8] = {
            Vector3(meshMin.x, meshMin.y, meshMin.z),
            Vector3(meshMax.x, meshMin.y, meshMin.z),
            Vector3(meshMin.x, meshMax.y, meshMin.z),
            Vector3(meshMax.x, meshMax.y, meshMin.z),
            Vector3(meshMin.x, meshMin.y, meshMax.z),
            Vector3(meshMax.x, meshMin.y, meshMax.z),
            Vector3(meshMin.x, meshMax.y, meshMax.z),
            Vector3(meshMax.x, meshMax.y, meshMax.z)
        };
        
        Matrix4x4 worldMatrix = transform.GetWorldMatrix();
        outMin = outMax = worldMatrix.TransformPoint(corners[0]);
        
        for (int i = 1; i < 8; ++i) {
            Vector3 worldCorner = worldMatrix.TransformPoint(corners[i]);
            outMin.x = std::min(outMin.x, worldCorner.x);
            outMin.y = std::min(outMin.y, worldCorner.y);
            outMin.z = std::min(outMin.z, worldCorner.z);
            outMax.x = std::max(outMax.x, worldCorner.x);
            outMax.y = std::max(outMax.y, worldCorner.y);
            outMax.z = std::max(outMax.z, worldCorner.z);
        }
    }

    float SceneObject::CalculateDistanceToCamera(const Vector3& cameraPosition) const {
        Vector3 objectCenter = transform.position;
        Vector3 diff = objectCenter - cameraPosition;
        return diff.Length();
    }

    ModelObject::ModelObject(const std::string& name) : name(name) {
    }

    std::vector<std::shared_ptr<SceneObject>> ModelObject::CreateSceneObjects() const {
        std::vector<std::shared_ptr<SceneObject>> sceneObjects;
        
        if (!model) {
            return sceneObjects;
        }
        
        sceneObjects.reserve(model->meshes.size());
        
        for (size_t i = 0; i < model->meshes.size(); ++i) {
            auto sceneObject = std::make_shared<SceneObject>(name + "_Mesh_" + std::to_string(i));
            sceneObject->SetMesh(model->meshes[i]);
            sceneObject->GetTransform() = transform;
            
            uint32_t materialIndex = model->meshes[i]->GetMaterialIndex();
            if (materialIndex < model->materials.size()) {
                sceneObject->SetMaterial(model->materials[materialIndex]);
            }
            
            sceneObject->SetVisible(visible);
            sceneObjects.push_back(sceneObject);
        }
        
        return sceneObjects;
    }

    void ModelObject::GetWorldBounds(Vector3& outMin, Vector3& outMax) const {
        if (!model) {
            outMin = outMax = transform.position;
            return;
        }
        
        Vector3 modelMin = model->minBounds;
        Vector3 modelMax = model->maxBounds;
        
        Vector3 corners[8] = {
            Vector3(modelMin.x, modelMin.y, modelMin.z),
            Vector3(modelMax.x, modelMin.y, modelMin.z),
            Vector3(modelMin.x, modelMax.y, modelMin.z),
            Vector3(modelMax.x, modelMax.y, modelMin.z),
            Vector3(modelMin.x, modelMin.y, modelMax.z),
            Vector3(modelMax.x, modelMin.y, modelMax.z),
            Vector3(modelMin.x, modelMax.y, modelMax.z),
            Vector3(modelMax.x, modelMax.y, modelMax.z)
        };
        
        Matrix4x4 worldMatrix = transform.GetWorldMatrix();
        outMin = outMax = worldMatrix.TransformPoint(corners[0]);
        
        for (int i = 1; i < 8; ++i) {
            Vector3 worldCorner = worldMatrix.TransformPoint(corners[i]);
            outMin.x = std::min(outMin.x, worldCorner.x);
            outMin.y = std::min(outMin.y, worldCorner.y);
            outMin.z = std::min(outMin.z, worldCorner.z);
            outMax.x = std::max(outMax.x, worldCorner.x);
            outMax.y = std::max(outMax.y, worldCorner.y);
            outMax.z = std::max(outMax.z, worldCorner.z);
        }
    }

} // namespace Athena