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
        Vector3& operator+=(const Vector3& v) { x += v.x; y += v.y; z += v.z; return *this; }
        Vector3& operator-=(const Vector3& v) { x -= v.x; y -= v.y; z -= v.z; return *this; }
        Vector3& operator*=(float s) { x *= s; y *= s; z *= s; return *this; }
        Vector3& operator/=(float s) { x /= s; y /= s; z /= s; return *this; }

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

        // ������2��i�������v�Z���ȗ��j
        float LengthSquared() const {
            return x * x + y * y + z * z;
        }

        // ���K��
        Vector3 Normalize() const {
            float len = Length();
            return (len > 0.0f) ? (*this / len) : Vector3(0, 0, 0);
        }

        // ���`���
        static Vector3 Lerp(const Vector3& a, const Vector3& b, float t) {
            return a * (1.0f - t) + b * t;
        }

        // ����
        static float Distance(const Vector3& a, const Vector3& b) {
            return (b - a).Length();
        }

        // ���˃x�N�g��
        Vector3 Reflect(const Vector3& normal) const {
            return *this - normal * (2.0f * Dot(normal));
        }
    };

    /**
     * @brief 4D�x�N�g���i�������W�j
     */
    struct Vector4 {
        float x, y, z, w;

        Vector4() : x(0), y(0), z(0), w(0) {}
        Vector4(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}
        Vector4(const Vector3& v, float w) : x(v.x), y(v.y), z(v.z), w(w) {}

        Vector3 ToVector3() const { return Vector3(x, y, z); }
    };

    /**
     * @brief 4x4�s��
     *
     * ���������C�A�E�g�F�s�D��iRow-major�j
     * m[�s][��]
     *
     * DirectX 12�ł́AC++���͍s�D��AHLSL���͗�D��Ƃ��Ĉ����B
     * ���̂��߁A�V�F�[�_�[�ɓn���O�ɓ]�u���K�v�ȏꍇ������B
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

        // �x�N�g���Ƃ̏�Z
        Vector4 operator*(const Vector4& v) const {
            Vector4 result;
            result.x = m[0][0] * v.x + m[0][1] * v.y + m[0][2] * v.z + m[0][3] * v.w;
            result.y = m[1][0] * v.x + m[1][1] * v.y + m[1][2] * v.z + m[1][3] * v.w;
            result.z = m[2][0] * v.x + m[2][1] * v.y + m[2][2] * v.z + m[2][3] * v.w;
            result.w = m[3][0] * v.x + m[3][1] * v.y + m[3][2] * v.z + m[3][3] * v.w;
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

        // �s�񎮁i3x3�����j
        float Determinant3x3(int row, int col) const {
            int rows[3], cols[3];
            int r = 0, c = 0;

            for (int i = 0; i < 4; ++i) {
                if (i != row) rows[r++] = i;
            }
            for (int i = 0; i < 4; ++i) {
                if (i != col) cols[c++] = i;
            }

            return m[rows[0]][cols[0]] * (m[rows[1]][cols[1]] * m[rows[2]][cols[2]] - m[rows[1]][cols[2]] * m[rows[2]][cols[1]])
                - m[rows[0]][cols[1]] * (m[rows[1]][cols[0]] * m[rows[2]][cols[2]] - m[rows[1]][cols[2]] * m[rows[2]][cols[0]])
                + m[rows[0]][cols[2]] * (m[rows[1]][cols[0]] * m[rows[2]][cols[1]] - m[rows[1]][cols[1]] * m[rows[2]][cols[0]]);
        }

        // �s��
        float Determinant() const {
            float det = 0.0f;
            for (int i = 0; i < 4; ++i) {
                float sign = (i % 2 == 0) ? 1.0f : -1.0f;
                det += sign * m[0][i] * Determinant3x3(0, i);
            }
            return det;
        }

        // �t�s��
        Matrix4x4 Inverse() const {
            Matrix4x4 result;
            float det = Determinant();

            if (std::abs(det) < 1e-6f) {
                // �t�s�񂪑��݂��Ȃ��ꍇ�͒P�ʍs���Ԃ�
                result.Identity();
                return result;
            }

            float invDet = 1.0f / det;

            for (int i = 0; i < 4; ++i) {
                for (int j = 0; j < 4; ++j) {
                    float sign = ((i + j) % 2 == 0) ? 1.0f : -1.0f;
                    result.m[j][i] = sign * Determinant3x3(i, j) * invDet;
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

        static Matrix4x4 Translation(const Vector3& v) {
            return Translation(v.x, v.y, v.z);
        }

        // �X�P�[���s��
        static Matrix4x4 Scaling(float x, float y, float z) {
            Matrix4x4 mat;
            mat.m[0][0] = x;
            mat.m[1][1] = y;
            mat.m[2][2] = z;
            return mat;
        }

        static Matrix4x4 Scaling(float s) {
            return Scaling(s, s, s);
        }

        static Matrix4x4 Scaling(const Vector3& v) {
            return Scaling(v.x, v.y, v.z);
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

        // �C�ӎ���]�s��iRodrigues�̉�]�����j
        static Matrix4x4 RotationAxis(const Vector3& axis, float angle) {
            Vector3 n = axis.Normalize();
            float c = std::cos(angle);
            float s = std::sin(angle);
            float t = 1.0f - c;

            Matrix4x4 mat;
            mat.m[0][0] = t * n.x * n.x + c;
            mat.m[0][1] = t * n.x * n.y - s * n.z;
            mat.m[0][2] = t * n.x * n.z + s * n.y;
            mat.m[0][3] = 0.0f;

            mat.m[1][0] = t * n.x * n.y + s * n.z;
            mat.m[1][1] = t * n.y * n.y + c;
            mat.m[1][2] = t * n.y * n.z - s * n.x;
            mat.m[1][3] = 0.0f;

            mat.m[2][0] = t * n.x * n.z - s * n.y;
            mat.m[2][1] = t * n.y * n.z + s * n.x;
            mat.m[2][2] = t * n.z * n.z + c;
            mat.m[2][3] = 0.0f;

            mat.m[3][0] = 0.0f;
            mat.m[3][1] = 0.0f;
            mat.m[3][2] = 0.0f;
            mat.m[3][3] = 1.0f;

            return mat;
        }

        // �r���[�s��i������W�n�j
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

        // �r���[�s��i�E����W�n�j
        static Matrix4x4 LookAtRH(const Vector3& eye, const Vector3& target, const Vector3& up) {
            Vector3 zAxis = (eye - target).Normalize();  // �O�����i�E��n�͋t�j
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

        // �f�t�H���g��LookAt�i������W�n�j
        static Matrix4x4 LookAt(const Vector3& eye, const Vector3& target, const Vector3& up) {
            return LookAtLH(eye, target, up);
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

        // �������e�s��i�E����W�n�j
        static Matrix4x4 PerspectiveFovRH(float fovY, float aspect, float nearZ, float farZ) {
            float h = 1.0f / std::tan(fovY * 0.5f);
            float w = h / aspect;

            Matrix4x4 mat;
            mat.m[0][0] = w;
            mat.m[1][1] = h;
            mat.m[2][2] = farZ / (nearZ - farZ);
            mat.m[2][3] = -1.0f;
            mat.m[3][2] = nearZ * farZ / (nearZ - farZ);
            mat.m[3][3] = 0.0f;

            return mat;
        }

        // �f�t�H���g��Perspective�i������W�n�j
        static Matrix4x4 Perspective(float fovY, float aspect, float nearZ, float farZ) {
            return PerspectiveFovLH(fovY, aspect, nearZ, farZ);
        }

        // ���ˉe�s��i������W�n�j
        static Matrix4x4 OrthographicLH(float width, float height, float nearZ, float farZ) {
            Matrix4x4 mat;
            mat.m[0][0] = 2.0f / width;
            mat.m[1][1] = 2.0f / height;
            mat.m[2][2] = 1.0f / (farZ - nearZ);
            mat.m[3][2] = -nearZ / (farZ - nearZ);
            return mat;
        }

        // ���ˉe�s��i�E����W�n�j
        static Matrix4x4 OrthographicRH(float width, float height, float nearZ, float farZ) {
            Matrix4x4 mat;
            mat.m[0][0] = 2.0f / width;
            mat.m[1][1] = 2.0f / height;
            mat.m[2][2] = 1.0f / (nearZ - farZ);
            mat.m[3][2] = nearZ / (nearZ - farZ);
            return mat;
        }

        // �I�t�Z���^�[���ˉe�i������W�n�j
        static Matrix4x4 OrthographicOffCenterLH(float left, float right, float bottom, float top, float nearZ, float farZ) {
            Matrix4x4 mat;
            mat.m[0][0] = 2.0f / (right - left);
            mat.m[1][1] = 2.0f / (top - bottom);
            mat.m[2][2] = 1.0f / (farZ - nearZ);
            mat.m[3][0] = -(right + left) / (right - left);
            mat.m[3][1] = -(top + bottom) / (top - bottom);
            mat.m[3][2] = -nearZ / (farZ - nearZ);
            return mat;
        }

        // �I�t�Z���^�[���ˉe�i�E����W�n�j
        static Matrix4x4 OrthographicOffCenterRH(float left, float right, float bottom, float top, float nearZ, float farZ) {
            Matrix4x4 mat;
            mat.m[0][0] = 2.0f / (right - left);
            mat.m[1][1] = 2.0f / (top - bottom);
            mat.m[2][2] = 1.0f / (nearZ - farZ);
            mat.m[3][0] = -(right + left) / (right - left);
            mat.m[3][1] = -(top + bottom) / (top - bottom);
            mat.m[3][2] = nearZ / (nearZ - farZ);
            return mat;
        }
    };

    // �萔
    constexpr float PI = 3.14159265358979323846f;
    constexpr float TWO_PI = 2.0f * PI;
    constexpr float HALF_PI = 0.5f * PI;
    constexpr float EPSILON = 1e-6f;

    // �x�����W�A���ɕϊ�
    inline float ToRadians(float degrees) {
        return degrees * (PI / 180.0f);
    }

    // ���W�A����x�ɕϊ�
    inline float ToDegrees(float radians) {
        return radians * (180.0f / PI);
    }

    // �l�̃N�����v
    inline float Clamp(float value, float min, float max) {
        return std::max(min, std::min(value, max));
    }

    // ���`���
    inline float Lerp(float a, float b, float t) {
        return a + (b - a) * t;
    }

    // ������ԁiSmoothstep�j
    inline float Smoothstep(float edge0, float edge1, float x) {
        float t = Clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
        return t * t * (3.0f - 2.0f * t);
    }

} // namespace Athena