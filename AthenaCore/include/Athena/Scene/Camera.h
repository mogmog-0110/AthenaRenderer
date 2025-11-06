// Camera.h - カメラ基底クラス
#pragma once

#include "Athena/Utils/Math.h"

namespace Athena {

    /**
     * @brief カメラの種類
     */
    enum class CameraType {
        FPS,        // 一人称視点カメラ（WASD移動 + マウス回転）
        Orbit       // 注視点中心カメラ（マウスドラッグで回転）
    };

    /**
     * @brief カメラ基底クラス
     *
     * View行列とProjection行列を管理し、
     * カメラの位置・向き・視野角などを制御する
     */
    class Camera {
    public:
        Camera();
        virtual ~Camera() = default;

        // ===== 更新 =====
        virtual void Update(float deltaTime) = 0;

        // ===== 行列取得 =====
        Matrix4x4 GetViewMatrix() const { return viewMatrix; }
        Matrix4x4 GetProjectionMatrix() const { return projectionMatrix; }
        Matrix4x4 GetViewProjectionMatrix() const { return projectionMatrix * viewMatrix; }

        // ===== カメラパラメータ =====
        Vector3 GetPosition() const { return position; }
        Vector3 GetForward() const { return forward; }
        Vector3 GetRight() const { return right; }
        Vector3 GetUp() const { return up; }

        void SetPosition(const Vector3& pos) { position = pos; UpdateViewMatrix(); }

        // ===== プロジェクション設定 =====
        void SetPerspective(float fovY, float aspectRatio, float nearZ, float farZ);
        void SetOrthographic(float width, float height, float nearZ, float farZ);

        // ===== パラメータアクセス =====
        float GetFOV() const { return fov; }
        float GetNearZ() const { return nearZ; }
        float GetFarZ() const { return farZ; }
        float GetAspectRatio() const { return aspectRatio; }

    protected:
        // ===== View行列更新（派生クラスで実装） =====
        virtual void UpdateViewMatrix() = 0;

        // ===== メンバ変数 =====
        // 位置と方向
        Vector3 position;
        Vector3 forward;   // 前方向ベクトル
        Vector3 right;     // 右方向ベクトル
        Vector3 up;        // 上方向ベクトル

        // 行列
        Matrix4x4 viewMatrix;
        Matrix4x4 projectionMatrix;

        // プロジェクションパラメータ
        float fov;          // 視野角（ラジアン）
        float aspectRatio;  // アスペクト比
        float nearZ;        // ニアクリップ面
        float farZ;         // ファークリップ面
    };

    // =====================================================
    // FPSカメラ（一人称視点）
    // =====================================================
    /**
     * @brief FPSカメラ
     *
     * WASD: 移動
     * マウス: 視点回転
     * Shift: 高速移動
     */
    class FPSCamera : public Camera {
    public:
        FPSCamera();

        void Update(float deltaTime) override;

        // ===== 入力処理 =====
        void OnMouseMove(float deltaX, float deltaY);
        void OnKeyPress(int key, bool isPressed);

        // ===== パラメータ設定 =====
        void SetMoveSpeed(float speed) { moveSpeed = speed; }
        void SetRotationSpeed(float speed) { rotationSpeed = speed; }
        void SetMouseSensitivity(float sensitivity) { mouseSensitivity = sensitivity; }

    protected:
        void UpdateViewMatrix() override;

    private:
        // 回転角度（オイラー角）
        float yaw;      // ヨー（Y軸回転）
        float pitch;    // ピッチ（X軸回転）

        // 移動速度
        float moveSpeed;
        float rotationSpeed;
        float mouseSensitivity;

        // 入力状態
        bool moveForward;
        bool moveBackward;
        bool moveLeft;
        bool moveRight;
        bool moveUp;
        bool moveDown;
        bool isSprinting;
    };

    // =====================================================
    // オービットカメラ（注視点中心回転）
    // =====================================================
    /**
     * @brief オービットカメラ
     *
     * マウスドラッグ: 回転
     * ホイール: ズーム
     * 常に注視点（target）を向く
     */
    class OrbitCamera : public Camera {
    public:
        OrbitCamera();

        void Update(float deltaTime) override;

        // ===== 入力処理 =====
        void OnMouseMove(float deltaX, float deltaY, bool isRotating);
        void OnMouseScroll(float delta);

        // ===== パラメータ設定 =====
        void SetTarget(const Vector3& target) { this->target = target; UpdateViewMatrix(); }
        void SetDistance(float distance) { this->distance = distance; UpdateViewMatrix(); }
        void SetRotationSpeed(float speed) { rotationSpeed = speed; }
        void SetZoomSpeed(float speed) { zoomSpeed = speed; }

        Vector3 GetTarget() const { return target; }
        float GetDistance() const { return distance; }

    protected:
        void UpdateViewMatrix() override;

    private:
        // 注視点
        Vector3 target;

        // 極座標パラメータ
        float distance;     // 注視点からの距離
        float yaw;          // ヨー角
        float pitch;        // ピッチ角

        // 速度
        float rotationSpeed;
        float zoomSpeed;

        // 制限
        float minDistance;
        float maxDistance;
        float minPitch;
        float maxPitch;
    };

} // namespace Athena