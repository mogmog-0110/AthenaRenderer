// Camera.h - �J�������N���X
#pragma once

#include "Athena/Utils/Math.h"

namespace Athena {

    /**
     * @brief �J�����̎��
     */
    enum class CameraType {
        FPS,        // ��l�̎��_�J�����iWASD�ړ� + �}�E�X��]�j
        Orbit       // �����_���S�J�����i�}�E�X�h���b�O�ŉ�]�j
    };

    /**
     * @brief �J�������N���X
     *
     * View�s���Projection�s����Ǘ����A
     * �J�����̈ʒu�E�����E����p�Ȃǂ𐧌䂷��
     */
    class Camera {
    public:
        Camera();
        virtual ~Camera() = default;

        // ===== �X�V =====
        virtual void Update(float deltaTime) = 0;

        // ===== �s��擾 =====
        Matrix4x4 GetViewMatrix() const { return viewMatrix; }
        Matrix4x4 GetProjectionMatrix() const { return projectionMatrix; }
        Matrix4x4 GetViewProjectionMatrix() const { return projectionMatrix * viewMatrix; }

        // ===== �J�����p�����[�^ =====
        Vector3 GetPosition() const { return position; }
        Vector3 GetForward() const { return forward; }
        Vector3 GetRight() const { return right; }
        Vector3 GetUp() const { return up; }

        void SetPosition(const Vector3& pos) { position = pos; UpdateViewMatrix(); }

        // ===== �v���W�F�N�V�����ݒ� =====
        void SetPerspective(float fovY, float aspectRatio, float nearZ, float farZ);
        void SetOrthographic(float width, float height, float nearZ, float farZ);

        // ===== �p�����[�^�A�N�Z�X =====
        float GetFOV() const { return fov; }
        float GetNearZ() const { return nearZ; }
        float GetFarZ() const { return farZ; }
        float GetAspectRatio() const { return aspectRatio; }

    protected:
        // ===== View�s��X�V�i�h���N���X�Ŏ����j =====
        virtual void UpdateViewMatrix() = 0;

        // ===== �����o�ϐ� =====
        // �ʒu�ƕ���
        Vector3 position;
        Vector3 forward;   // �O�����x�N�g��
        Vector3 right;     // �E�����x�N�g��
        Vector3 up;        // ������x�N�g��

        // �s��
        Matrix4x4 viewMatrix;
        Matrix4x4 projectionMatrix;

        // �v���W�F�N�V�����p�����[�^
        float fov;          // ����p�i���W�A���j
        float aspectRatio;  // �A�X�y�N�g��
        float nearZ;        // �j�A�N���b�v��
        float farZ;         // �t�@�[�N���b�v��
    };

    // =====================================================
    // FPS�J�����i��l�̎��_�j
    // =====================================================
    /**
     * @brief FPS�J����
     *
     * WASD: �ړ�
     * �}�E�X: ���_��]
     * Shift: �����ړ�
     */
    class FPSCamera : public Camera {
    public:
        FPSCamera();

        void Update(float deltaTime) override;

        // ===== ���͏��� =====
        void OnMouseMove(float deltaX, float deltaY);
        void OnMouseScroll(float delta);
        void OnKeyPress(int key, bool isPressed);

        // ===== �p�����[�^�ݒ� =====
        void SetMoveSpeed(float speed) { moveSpeed = speed; }
        void SetRotationSpeed(float speed) { rotationSpeed = speed; }
        void SetMouseSensitivity(float sensitivity) { mouseSensitivity = sensitivity; }
        
        // ===== リセット機能 =====
        void ResetToDefaultPosition();
        void ResetRotation();

    protected:
        void UpdateViewMatrix() override;

    private:
        // ��]�p�x�i�I�C���[�p�j
        float yaw;      // ���[�iY����]�j
        float pitch;    // �s�b�`�iX����]�j

        // �ړ����x
        float moveSpeed;
        float rotationSpeed;
        float mouseSensitivity;
        float scrollSensitivity;

        // ���͏��
        bool moveForward;
        bool moveBackward;
        bool moveLeft;
        bool moveRight;
        bool moveUp;
        bool moveDown;
        bool isSprinting;
    };

    // =====================================================
    // �I�[�r�b�g�J�����i�����_���S��]�j
    // =====================================================
    /**
     * @brief �I�[�r�b�g�J����
     *
     * �}�E�X�h���b�O: ��]
     * �z�C�[��: �Y�[��
     * ��ɒ����_�itarget�j������
     */
    class OrbitCamera : public Camera {
    public:
        OrbitCamera();

        void Update(float deltaTime) override;

        // ===== ���͏��� =====
        void OnMouseMove(float deltaX, float deltaY, bool isRotating);
        void OnMouseScroll(float delta);

        // ===== �p�����[�^�ݒ� =====
        void SetTarget(const Vector3& target) { this->target = target; UpdateViewMatrix(); }
        void SetDistance(float distance) { this->distance = distance; UpdateViewMatrix(); }
        void SetRotationSpeed(float speed) { rotationSpeed = speed; }
        void SetZoomSpeed(float speed) { zoomSpeed = speed; }

        Vector3 GetTarget() const { return target; }
        float GetDistance() const { return distance; }

    protected:
        void UpdateViewMatrix() override;

    private:
        // �����_
        Vector3 target;

        // �ɍ��W�p�����[�^
        float distance;     // �����_����̋���
        float yaw;          // ���[�p
        float pitch;        // �s�b�`�p

        // ���x
        float rotationSpeed;
        float zoomSpeed;

        // ����
        float minDistance;
        float maxDistance;
        float minPitch;
        float maxPitch;
    };

} // namespace Athena