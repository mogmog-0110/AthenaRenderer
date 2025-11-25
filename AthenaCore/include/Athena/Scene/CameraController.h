#pragma once

#include "Camera.h"
#include "../Utils/Math.h"
#include <memory>
#include <vector>
#include <string>
#include <unordered_map>

namespace Athena {

    struct CameraKeyframe {
        float time = 0.0f;
        Vector3 position = Vector3(0, 0, 0);
        Vector3 target = Vector3(0, 0, 0);
        float yaw = 0.0f;
        float pitch = 0.0f;
        float fov = 60.0f;
    };

    struct CameraPreset {
        Vector3 position = Vector3(0, 0, 0);
        Vector3 target = Vector3(0, 0, 0);
        float yaw = 0.0f;
        float pitch = 0.0f;
        float fov = 60.0f;
        std::string name;
    };

    class CameraAnimation {
    public:
        CameraAnimation() = default;
        
        void AddKeyframe(const CameraKeyframe& keyframe);
        void ClearKeyframes();
        void SetDuration(float duration);
        void SetLooping(bool loop);
        
        CameraKeyframe Sample(float time) const;
        bool IsFinished(float time) const;
        float GetDuration() const { return duration; }
        size_t GetKeyframeCount() const { return keyframes.size(); }
        
    private:
        std::vector<CameraKeyframe> keyframes;
        float duration = 0.0f;
        bool looping = false;
        
        // Linear interpolation between keyframes
        CameraKeyframe InterpolateKeyframes(const CameraKeyframe& a, const CameraKeyframe& b, float t) const;
    };

    class CameraController {
    public:
        explicit CameraController(std::shared_ptr<Camera> camera);
        ~CameraController() = default;

        void Update(float deltaTime);
        
        // Basic camera control
        void SetPosition(const Vector3& position);
        void SetTarget(const Vector3& target);
        void LookAt(const Vector3& target);
        void SetFOV(float fov);
        void SetNearFar(float nearPlane, float farPlane);
        
        // Animation control
        void StartAnimation(std::shared_ptr<CameraAnimation> animation);
        void StopAnimation();
        bool IsAnimating() const;
        float GetAnimationProgress() const;
        
        // Preset management
        void AddPreset(const std::string& name, const CameraPreset& preset);
        void LoadPreset(const std::string& name, float transitionTime = 0.0f);
        std::vector<std::string> GetPresetNames() const;
        
        // Movement settings
        void SetSpeed(float speed);
        void SetSensitivity(float sensitivity);
        float GetSpeed() const { return speed; }
        float GetSensitivity() const { return sensitivity; }
        
        // Smooth transitions
        void SetTransitionTime(float time) { transitionTime = time; }
        bool IsTransitioning() const { return isTransitioning; }
        
    private:
        std::shared_ptr<Camera> camera;
        
        // Animation state
        std::shared_ptr<CameraAnimation> currentAnimation;
        float animationTime = 0.0f;
        bool isAnimating = false;
        
        // Transition state
        bool isTransitioning = false;
        float transitionTime = 1.0f;
        float currentTransitionTime = 0.0f;
        CameraKeyframe transitionStart;
        CameraKeyframe transitionTarget;
        
        // Settings
        float speed = 1.0f;
        float sensitivity = 1.0f;
        std::unordered_map<std::string, CameraPreset> presets;
        
        // Helper methods
        void ApplyKeyframe(const CameraKeyframe& keyframe);
        CameraKeyframe GetCurrentKeyframe() const;
        void StartTransition(const CameraKeyframe& target, float duration);
    };

} // namespace Athena