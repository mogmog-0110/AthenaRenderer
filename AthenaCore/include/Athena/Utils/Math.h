#pragma once

#include <cmath>
#include <algorithm>

namespace Athena {

    /**
     * @brief 3D�x�N�g��
     */
    struct Vector3 {
        float x, y, z;

        Vector3() : x(0), y(0), z(0) {}
        Vector3(float x, float y, float z) : x(x), y(y), z(z) {}

        // ���Z�q
        Vector3 operator+(const Vector3& v) const { return Vector3(x + v.x, y + v.y, z + v.z); }
        Vector3 operator-(const Vector3& v) const { return Vector3(x - v.x, y - v.y, z - v.z); }
        Vector3 operator*(float s) const { return Vector3(x * s, y * s, z * s); }
        Vector3 operator/(float s) const { return Vector3(x / s, y / s, z / s); }

        // ����
        float Dot(const Vector3& v) const {
            return x * v.x + y * v.y + z * v.z;
        }

        // �O��
        Vector3 Cross(const Vector3& v) const {
            return Vector3(
                y * v.z - z * v.y,
                z * v.x - x * v.z,
                x * v.y - y * v.x
            );
        }

        // ����
        float Length() const {
            return std::sqrt(x * x + y * y + z * z);
        }

        // ���K��
        Vector3 Normalize() const {
            float len = Length();
            return (len > 0.0f) ? (*this / len) : Vector3(0, 0, 0);
        }
    };

    /**
     * @brief 4x4�s��
     *
     * ���������C�A�E�g�F�s�D��iRow-major�j
     * m[�s][��]
     */
    struct Matrix4x4 {
        float m[4][4];

        Matrix4x4() {
            Identity();
        }

        // �P�ʍs��
        void Identity() {
            for (int i = 0; i < 4; ++i) {
                for (int j = 0; j < 4; ++j) {
                    m[i][j] = (i == j) ? 1.0f : 0.0f;
                }
            }
        }

        // �s��̏�Z
        Matrix4x4 operator*(const Matrix4x4& other) const {
            Matrix4x4 result;
            for (int i = 0; i < 4; ++i) {
                for (int j = 0; j < 4; ++j) {
                    result.m[i][j] = 0.0f;
                    for (int k = 0; k < 4; ++k) {
                        result.m[i][j] += m[i][k] * other.m[k][j];
                    }
                }
            }
            return result;
        }

        // �]�u�iHLSL�p�ɗ�D��ɕϊ��j
        Matrix4x4 Transpose() const {
            Matrix4x4 result;
            for (int i = 0; i < 4; ++i) {
                for (int j = 0; j < 4; ++j) {
                    result.m[i][j] = m[j][i];
                }
            }
            return result;
        }

        // ���s�ړ��s��
        static Matrix4x4 Translation(float x, float y, float z) {
            Matrix4x4 mat;
            mat.m[0][3] = x;
            mat.m[1][3] = y;
            mat.m[2][3] = z;
            return mat;
        }

        // �X�P�[���s��
        static Matrix4x4 Scaling(float x, float y, float z) {
            Matrix4x4 mat;
            mat.m[0][0] = x;
            mat.m[1][1] = y;
            mat.m[2][2] = z;
            return mat;
        }

        // X����]�s��
        static Matrix4x4 RotationX(float angle) {
            Matrix4x4 mat;
            float c = std::cos(angle);
            float s = std::sin(angle);
            mat.m[1][1] = c;
            mat.m[1][2] = -s;
            mat.m[2][1] = s;
            mat.m[2][2] = c;
            return mat;
        }

        // Y����]�s��
        static Matrix4x4 RotationY(float angle) {
            Matrix4x4 mat;
            float c = std::cos(angle);
            float s = std::sin(angle);
            mat.m[0][0] = c;
            mat.m[0][2] = s;
            mat.m[2][0] = -s;
            mat.m[2][2] = c;
            return mat;
        }

        // Z����]�s��
        static Matrix4x4 RotationZ(float angle) {
            Matrix4x4 mat;
            float c = std::cos(angle);
            float s = std::sin(angle);
            mat.m[0][0] = c;
            mat.m[0][1] = -s;
            mat.m[1][0] = s;
            mat.m[1][1] = c;
            return mat;
        }

        // �r���[�s��iLookAt�j
        static Matrix4x4 LookAtLH(const Vector3& eye, const Vector3& target, const Vector3& up) {
            Vector3 zAxis = (target - eye).Normalize();  // �O����
            Vector3 xAxis = up.Cross(zAxis).Normalize(); // �E����
            Vector3 yAxis = zAxis.Cross(xAxis);          // �����

            Matrix4x4 mat;
            mat.m[0][0] = xAxis.x;
            mat.m[0][1] = yAxis.x;
            mat.m[0][2] = zAxis.x;
            mat.m[0][3] = 0.0f;

            mat.m[1][0] = xAxis.y;
            mat.m[1][1] = yAxis.y;
            mat.m[1][2] = zAxis.y;
            mat.m[1][3] = 0.0f;

            mat.m[2][0] = xAxis.z;
            mat.m[2][1] = yAxis.z;
            mat.m[2][2] = zAxis.z;
            mat.m[2][3] = 0.0f;

            mat.m[3][0] = -xAxis.Dot(eye);
            mat.m[3][1] = -yAxis.Dot(eye);
            mat.m[3][2] = -zAxis.Dot(eye);
            mat.m[3][3] = 1.0f;

            return mat;
        }

        // �������e�s��i������W�n�j
        static Matrix4x4 PerspectiveFovLH(float fovY, float aspect, float nearZ, float farZ) {
            float h = 1.0f / std::tan(fovY * 0.5f);
            float w = h / aspect;

            Matrix4x4 mat;
            mat.m[0][0] = w;
            mat.m[1][1] = h;
            mat.m[2][2] = farZ / (farZ - nearZ);
            mat.m[2][3] = 1.0f;
            mat.m[3][2] = -nearZ * farZ / (farZ - nearZ);
            mat.m[3][3] = 0.0f;

            return mat;
        }
    };

    // �萔
    constexpr float PI = 3.14159265358979323846f;

    // �x�����W�A���ɕϊ�
    inline float ToRadians(float degrees) {
        return degrees * (PI / 180.0f);
    }

    // ���W�A����x�ɕϊ�
    inline float ToDegrees(float radians) {
        return radians * (180.0f / PI);
    }

} // namespace Athena