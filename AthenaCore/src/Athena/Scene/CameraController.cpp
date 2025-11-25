#include "Athena/Scene/CameraController.h"
#include "Athena/Utils/Logger.h"
#include <string>
#include <algorithm>
#include <cmath>

namespace Athena {

// CameraAnimation implementation
void CameraAnimation::AddKeyframe(const CameraKeyframe& keyframe) {
    keyframes.push_back(keyframe);
    // Sort keyframes by time
    std::sort(keyframes.begin(), keyframes.end(), 
        [](const CameraKeyframe& a, const CameraKeyframe& b) {
            return a.time < b.time;
        });
}

void CameraAnimation::ClearKeyframes() {
    keyframes.clear();
}

void CameraAnimation::SetDuration(float duration) {
    this->duration = duration;
}

void CameraAnimation::SetLooping(bool loop) {
    this->looping = loop;
}

CameraKeyframe CameraAnimation::Sample(float time) const {
    if (keyframes.empty()) {
        return CameraKeyframe();
    }
    
    // Handle looping
    if (looping && duration > 0.0f) {
        time = fmod(time, duration);
    }
    
    // Clamp time
    time = std::max(0.0f, std::min(time, duration));
    
    // Find surrounding keyframes
    if (keyframes.size() == 1) {
        return keyframes[0];
    }
    
    for (size_t i = 0; i < keyframes.size() - 1; ++i) {
        if (time >= keyframes[i].time && time <= keyframes[i + 1].time) {
            float t = (time - keyframes[i].time) / (keyframes[i + 1].time - keyframes[i].time);
            return InterpolateKeyframes(keyframes[i], keyframes[i + 1], t);
        }
    }
    
    // Return last keyframe if time is beyond range
    return keyframes.back();
}

bool CameraAnimation::IsFinished(float time) const {
    return !looping && time >= duration;
}

CameraKeyframe CameraAnimation::InterpolateKeyframes(const CameraKeyframe& a, const CameraKeyframe& b, float t) const {
    CameraKeyframe result;
    result.time = a.time + t * (b.time - a.time);
    
    // Linear interpolation for position and target
    result.position = a.position + (b.position - a.position) * t;
    result.target = a.target + (b.target - a.target) * t;
    
    // Interpolate angles (handle wraparound)
    result.yaw = a.yaw + t * (b.yaw - a.yaw);
    result.pitch = a.pitch + t * (b.pitch - a.pitch);
    result.fov = a.fov + t * (b.fov - a.fov);
    
    return result;
}

// CameraController implementation
CameraController::CameraController(std::shared_ptr<Camera> camera) 
    : camera(camera), speed(5.0f), sensitivity(0.1f) {
    Logger::Info("CameraController created with animation support");
    
    // Add default presets
    CameraPreset frontView;
    frontView.name = "Front";
    frontView.position = Vector3(0, 0, -5);
    frontView.target = Vector3(0, 0, 0);
    AddPreset("Front", frontView);
    
    CameraPreset topView;
    topView.name = "Top";
    topView.position = Vector3(0, 5, 0);
    topView.target = Vector3(0, 0, 0);
    AddPreset("Top", topView);
    
    CameraPreset rightView;
    rightView.name = "Right";
    rightView.position = Vector3(5, 0, 0);
    rightView.target = Vector3(0, 0, 0);
    AddPreset("Right", rightView);
}

void CameraController::Update(float deltaTime) {
    // Update animation
    if (isAnimating && currentAnimation) {
        animationTime += deltaTime;
        
        CameraKeyframe keyframe = currentAnimation->Sample(animationTime);
        ApplyKeyframe(keyframe);
        
        if (currentAnimation->IsFinished(animationTime)) {
            StopAnimation();
        }
    }
    
    // Update transition
    if (isTransitioning) {
        currentTransitionTime += deltaTime;
        float t = std::min(1.0f, currentTransitionTime / transitionTime);
        
        CameraKeyframe current;
        current.position = transitionStart.position + (transitionTarget.position - transitionStart.position) * t;
        current.target = transitionStart.target + (transitionTarget.target - transitionStart.target) * t;
        current.yaw = transitionStart.yaw + (transitionTarget.yaw - transitionStart.yaw) * t;
        current.pitch = transitionStart.pitch + (transitionTarget.pitch - transitionStart.pitch) * t;
        current.fov = transitionStart.fov + (transitionTarget.fov - transitionStart.fov) * t;
        
        ApplyKeyframe(current);
        
        if (t >= 1.0f) {
            isTransitioning = false;
        }
    }
    
    // Update camera
    if (camera) {
        camera->Update(deltaTime);
    }
}

void CameraController::SetPosition(const Vector3& position) {
    if (camera) {
        camera->SetPosition(position);
    }
}

void CameraController::SetTarget(const Vector3& target) {
    // This would require extending the Camera base class
    Logger::Info("SetTarget called with ({:.2f}, {:.2f}, {:.2f})", target.x, target.y, target.z);
}

void CameraController::LookAt(const Vector3& target) {
    Logger::Info("LookAt called with ({:.2f}, {:.2f}, {:.2f})", target.x, target.y, target.z);
}

void CameraController::SetFOV(float fov) {
    Logger::Info("SetFOV called with {:.2f} degrees", fov);
}

void CameraController::SetNearFar(float nearPlane, float farPlane) {
    Logger::Info("SetNearFar called with near={:.2f}, far={:.2f}", nearPlane, farPlane);
}

void CameraController::StartAnimation(std::shared_ptr<CameraAnimation> animation) {
    if (!animation || animation->GetKeyframeCount() == 0) {
        Logger::Warning("Cannot start animation: invalid or empty animation");
        return;
    }
    
    currentAnimation = animation;
    animationTime = 0.0f;
    isAnimating = true;
    isTransitioning = false; // Stop any transition
    
    Logger::Info("Started camera animation with {} keyframes, duration {:.2f}s", 
                animation->GetKeyframeCount(), animation->GetDuration());
}

void CameraController::StopAnimation() {
    isAnimating = false;
    currentAnimation.reset();
    animationTime = 0.0f;
    Logger::Info("Stopped camera animation");
}

bool CameraController::IsAnimating() const {
    return isAnimating;
}

float CameraController::GetAnimationProgress() const {
    if (!isAnimating || !currentAnimation) {
        return 0.0f;
    }
    
    float duration = currentAnimation->GetDuration();
    return duration > 0.0f ? std::min(1.0f, animationTime / duration) : 1.0f;
}

void CameraController::AddPreset(const std::string& name, const CameraPreset& preset) {
    presets[name] = preset;
    Logger::Info("Added camera preset '{}' at ({:.2f}, {:.2f}, {:.2f})", 
                name, preset.position.x, preset.position.y, preset.position.z);
}

void CameraController::LoadPreset(const std::string& name, float transitionTime) {
    auto it = presets.find(name);
    if (it == presets.end()) {
        Logger::Warning("Camera preset '{}' not found", name);
        return;
    }
    
    const CameraPreset& preset = it->second;
    
    CameraKeyframe target;
    target.position = preset.position;
    target.target = preset.target;
    target.yaw = preset.yaw;
    target.pitch = preset.pitch;
    target.fov = preset.fov;
    
    if (transitionTime > 0.0f) {
        StartTransition(target, transitionTime);
    } else {
        ApplyKeyframe(target);
    }
    
    Logger::Info("Loaded camera preset '{}'", name);
}

std::vector<std::string> CameraController::GetPresetNames() const {
    std::vector<std::string> names;
    names.reserve(presets.size());
    
    for (const auto& pair : presets) {
        names.push_back(pair.first);
    }
    
    return names;
}

void CameraController::SetSpeed(float speed) {
    this->speed = std::max(0.1f, speed);
}

void CameraController::SetSensitivity(float sensitivity) {
    this->sensitivity = std::max(0.01f, sensitivity);
}

void CameraController::ApplyKeyframe(const CameraKeyframe& keyframe) {
    if (camera) {
        camera->SetPosition(keyframe.position);
        // Additional camera properties would be applied here in a full implementation
    }
}

CameraKeyframe CameraController::GetCurrentKeyframe() const {
    CameraKeyframe current;
    if (camera) {
        current.position = camera->GetPosition();
        // Get other properties from camera
    }
    return current;
}

void CameraController::StartTransition(const CameraKeyframe& target, float duration) {
    transitionStart = GetCurrentKeyframe();
    transitionTarget = target;
    transitionTime = duration;
    currentTransitionTime = 0.0f;
    isTransitioning = true;
    isAnimating = false; // Stop any animation
}

} // namespace Athena