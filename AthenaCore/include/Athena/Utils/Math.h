#pragma once

#include <cmath>
#include <algorithm>

namespace Athena {

    /**
     * @brief 3Dベクトル
     */
    struct Vector3 {
        float x, y, z;

        Vector3() : x(0), y(0), z(0) {}
        Vector3(float x, float y, float z) : x(x), y(y), z(z) {}

        // 演算子
        Vector3 operator+(const Vector3& v) const { return Vector3(x + v.x, y + v.y, z + v.z); }
        Vector3 operator-(const Vector3& v) const { return Vector3(x - v.x, y - v.y, z - v.z); }
        Vector3 operator*(float s) const { return Vector3(x * s, y * s, z * s); }
        Vector3 operator/(float s) const { return Vector3(x / s, y / s, z / s); }

        // 内積
        float Dot(const Vector3& v) const {
            return x * v.x + y * v.y + z * v.z;
        }

        // 外積
        Vector3 Cross(const Vector3& v) const {
            return Vector3(
                y * v.z - z * v.y,
                z * v.x - x * v.z,
                x * v.y - y * v.x
            );
        }

        // 長さ
        float Length() const {
            return std::sqrt(x * x + y * y + z * z);
        }

        // 正規化
        Vector3 Normalize() const {
            float len = Length();
            return (len > 0.0f) ? (*this / len) : Vector3(0, 0, 0);
        }
    };

    /**
     * @brief 4x4行列
     *
     * メモリレイアウト：行優先（Row-major）
     * m[行][列]
     */
    struct Matrix4x4 {
        float m[4][4];

        Matrix4x4() {
            Identity();
        }

        // 単位行列
        void Identity() {
            for (int i = 0; i < 4; ++i) {
                for (int j = 0; j < 4; ++j) {
                    m[i][j] = (i == j) ? 1.0f : 0.0f;
                }
            }
        }

        // 行列の乗算
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

        // 転置（HLSL用に列優先に変換）
        Matrix4x4 Transpose() const {
            Matrix4x4 result;
            for (int i = 0; i < 4; ++i) {
                for (int j = 0; j < 4; ++j) {
                    result.m[i][j] = m[j][i];
                }
            }
            return result;
        }

        // 平行移動行列
        static Matrix4x4 Translation(float x, float y, float z) {
            Matrix4x4 mat;
            mat.m[0][3] = x;
            mat.m[1][3] = y;
            mat.m[2][3] = z;
            return mat;
        }

        // スケール行列
        static Matrix4x4 Scaling(float x, float y, float z) {
            Matrix4x4 mat;
            mat.m[0][0] = x;
            mat.m[1][1] = y;
            mat.m[2][2] = z;
            return mat;
        }

        // X軸回転行列
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

        // Y軸回転行列
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

        // Z軸回転行列
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

        // ビュー行列（LookAt）
        static Matrix4x4 LookAtLH(const Vector3& eye, const Vector3& target, const Vector3& up) {
            Vector3 zAxis = (target - eye).Normalize();  // 前方向
            Vector3 xAxis = up.Cross(zAxis).Normalize(); // 右方向
            Vector3 yAxis = zAxis.Cross(xAxis);          // 上方向

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

        // 透視投影行列（左手座標系）
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

    // 定数
    constexpr float PI = 3.14159265358979323846f;

    // 度をラジアンに変換
    inline float ToRadians(float degrees) {
        return degrees * (PI / 180.0f);
    }

    // ラジアンを度に変換
    inline float ToDegrees(float radians) {
        return radians * (180.0f / PI);
    }

} // namespace Athena