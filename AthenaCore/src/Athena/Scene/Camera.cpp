// Camera.cpp - カメラクラスの実装
#define NOMINMAX
#include "Athena/Scene/Camera.h"
#include "Athena/Utils/Logger.h"
#include <Windows.h>
#include <algorithm>
#include <cmath>

namespace Athena {

    // =====================================================
    // Camera 基底クラス
    // =====================================================

    Camera::Camera()
        : position(0.0f, 0.0f, 0.0f)
        , forward(0.0f, 0.0f, 1.0f)
        , right(1.0f, 0.0f, 0.0f)
        , up(0.0f, 1.0f, 0.0f)
        , fov(3.14159f / 4.0f)  // 45度
        , aspectRatio(16.0f / 9.0f)
        , nearZ(0.1f)
        , farZ(1000.0f)
    {
        // viewMatrixを単位行列で初期化
        viewMatrix.Identity();

        // projectionMatrixを設定
        projectionMatrix = Matrix4x4::Perspective(fov, aspectRatio, nearZ, farZ);
    }

    void Camera::SetPerspective(float fovY, float aspectRatio, float nearZ, float farZ) {
        this->fov = fovY;
        this->aspectRatio = aspectRatio;
        this->nearZ = nearZ;
        this->farZ = farZ;
        projectionMatrix = Matrix4x4::Perspective(fovY, aspectRatio, nearZ, farZ);
    }

    void Camera::SetOrthographic(float width, float height, float nearZ, float farZ) {
        this->nearZ = nearZ;
        this->farZ = farZ;
        // OrthographicLH を使用（左手座標系）
        projectionMatrix = Matrix4x4::OrthographicLH(width, height, nearZ, farZ);
    }

    // =====================================================
    // FPSカメラ
    // =====================================================

    FPSCamera::FPSCamera()
        : yaw(0.0f)
        , pitch(0.0f)
        , moveSpeed(5.0f)
        , rotationSpeed(0.1f)
        , mouseSensitivity(0.002f)
        , moveForward(false)
        , moveBackward(false)
        , moveLeft(false)
        , moveRight(false)
        , moveUp(false)
        , moveDown(false)
        , isSprinting(false)
    {
        position = Vector3(0.0f, 2.0f, -5.0f);
        UpdateViewMatrix();
    }

    void FPSCamera::Update(float deltaTime) {
        // 移動速度計算（Shiftで高速化）
        float speed = moveSpeed * deltaTime;
        if (isSprinting) {
            speed *= 3.0f;
        }

        // WASD移動
        Vector3 movement(0.0f, 0.0f, 0.0f);
        if (moveForward)  movement = movement + forward * speed;
        if (moveBackward) movement = movement - forward * speed;
        if (moveRight)    movement = movement + right * speed;
        if (moveLeft)     movement = movement - right * speed;

        // 上下移動（Space/Ctrl）
        if (moveUp)   movement.y += speed;
        if (moveDown) movement.y -= speed;

        position = position + movement;
        UpdateViewMatrix();
    }

    void FPSCamera::OnMouseMove(float deltaX, float deltaY) {
        // マウス移動量を角度変化に変換
        yaw += deltaX * mouseSensitivity;
        pitch += deltaY * mouseSensitivity;

        // ピッチ角を-89度〜+89度に制限（真上・真下を向きすぎないように）
        const float maxPitch = 1.553f;  // 約89度
        pitch = std::clamp(pitch, -maxPitch, maxPitch);

        UpdateViewMatrix();
    }

    void FPSCamera::OnKeyPress(int key, bool isPressed) {
        switch (key) {
        case 'W': case 'w': moveForward = isPressed; break;
        case 'S': case 's': moveBackward = isPressed; break;
        case 'A': case 'a': moveLeft = isPressed; break;
        case 'D': case 'd': moveRight = isPressed; break;
        case VK_SPACE:      moveUp = isPressed; break;      // Space
        case VK_CONTROL:    moveDown = isPressed; break;    // Ctrl
        case VK_SHIFT:      isSprinting = isPressed; break; // Shift
        }
    }

    void FPSCamera::UpdateViewMatrix() {
        // オイラー角から方向ベクトルを計算
        float cosYaw = std::cos(yaw);
        float sinYaw = std::sin(yaw);
        float cosPitch = std::cos(pitch);
        float sinPitch = std::sin(pitch);

        // 前方向ベクトル
        forward.x = cosYaw * cosPitch;
        forward.y = sinPitch;
        forward.z = sinYaw * cosPitch;
        forward = forward.Normalize();

        // 右方向ベクトル（前方向ベクトルとワールドY軸の外積）
        Vector3 worldUp(0.0f, 1.0f, 0.0f);
        right = forward.Cross(worldUp).Normalize();

        // 上方向ベクトル（右方向と前方向の外積）
        up = right.Cross(forward).Normalize();

        // View行列を計算
        viewMatrix = Matrix4x4::LookAt(position, position + forward, up);
    }

    // =====================================================
    // オービットカメラ
    // =====================================================

    OrbitCamera::OrbitCamera()
        : target(0.0f, 0.0f, 0.0f)
        , distance(10.0f)
        , yaw(0.0f)
        , pitch(0.3f)  // 少し上から見下ろす
        , rotationSpeed(0.005f)
        , zoomSpeed(1.0f)
        , minDistance(1.0f)
        , maxDistance(100.0f)
        , minPitch(-1.4f)  // 約-80度
        , maxPitch(1.4f)   // 約+80度
    {
        UpdateViewMatrix();
    }

    void OrbitCamera::Update(float deltaTime) {
        // 特に更新することはない（入力があったときに更新）
    }

    void OrbitCamera::OnMouseMove(float deltaX, float deltaY, bool isRotating) {
        if (!isRotating) return;

        // マウス移動を角度に変換
        yaw -= deltaX * rotationSpeed;
        pitch += deltaY * rotationSpeed;

        // ピッチ角を制限
        pitch = std::clamp(pitch, minPitch, maxPitch);

        UpdateViewMatrix();
    }

    void OrbitCamera::OnMouseScroll(float delta) {
        // ズーム（距離変更）
        distance -= delta * zoomSpeed;
        distance = std::clamp(distance, minDistance, maxDistance);

        UpdateViewMatrix();
    }

    void OrbitCamera::UpdateViewMatrix() {
        // 極座標から直交座標へ変換
        float cosYaw = std::cos(yaw);
        float sinYaw = std::sin(yaw);
        float cosPitch = std::cos(pitch);
        float sinPitch = std::sin(pitch);

        // カメラ位置を計算（注視点から distance 離れた位置）
        position.x = target.x + distance * cosPitch * cosYaw;
        position.y = target.y + distance * sinPitch;
        position.z = target.z + distance * cosPitch * sinYaw;

        // View行列を計算（常にtargetを見る）
        Vector3 worldUp(0.0f, 1.0f, 0.0f);
        viewMatrix = Matrix4x4::LookAt(position, target, worldUp);

        // 方向ベクトルも更新
        forward = (target - position).Normalize();
        right = forward.Cross(worldUp).Normalize();
        up = right.Cross(forward).Normalize();
    }

} // namespace Athena