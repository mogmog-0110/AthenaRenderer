#pragma once

#define NOMINMAX
#include <cmath>
#include <algorithm>

namespace Athena {

    /**
     * @brief 2Dベクトル
     */
    struct Vector2 {
        float x, y;

        Vector2() : x(0), y(0) {}
        Vector2(float x, float y) : x(x), y(y) {}

        // 演算子
        Vector2 operator+(const Vector2& v) const { return Vector2(x + v.x, y + v.y); }
        Vector2 operator-(const Vector2& v) const { return Vector2(x - v.x, y - v.y); }
        Vector2 operator*(float s) const { return Vector2(x * s, y * s); }
        Vector2 operator/(float s) const { return Vector2(x / s, y / s); }
        Vector2& operator+=(const Vector2& v) { x += v.x; y += v.y; return *this; }
        Vector2& operator-=(const Vector2& v) { x -= v.x; y -= v.y; return *this; }
        Vector2& operator*=(float s) { x *= s; y *= s; return *this; }
        Vector2& operator/=(float s) { x /= s; y /= s; return *this; }

        // 内積
        float Dot(const Vector2& v) const {
            return x * v.x + y * v.y;
        }

        // 長さ
        float Length() const {
            return std::sqrt(x * x + y * y);
        }

        // 長さの2乗
        float LengthSquared() const {
            return x * x + y * y;
        }

        // 正規化
        Vector2 Normalize() const {
            float len = Length();
            if (len > 0.0f) {
                return Vector2(x / len, y / len);
            }
            return Vector2(0, 0);
        }
    };

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
        Vector3& operator+=(const Vector3& v) { x += v.x; y += v.y; z += v.z; return *this; }
        Vector3& operator-=(const Vector3& v) { x -= v.x; y -= v.y; z -= v.z; return *this; }
        Vector3& operator*=(float s) { x *= s; y *= s; z *= s; return *this; }
        Vector3& operator/=(float s) { x /= s; y /= s; z /= s; return *this; }

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

        // 長さの2乗（平方根計算を省略）
        float LengthSquared() const {
            return x * x + y * y + z * z;
        }

        // 正規化
        Vector3 Normalize() const {
            float len = Length();
            return (len > 0.0f) ? (*this / len) : Vector3(0, 0, 0);
        }

        // 線形補間
        static Vector3 Lerp(const Vector3& a, const Vector3& b, float t) {
            return a * (1.0f - t) + b * t;
        }

        // 距離
        static float Distance(const Vector3& a, const Vector3& b) {
            return (b - a).Length();
        }

        // 反射ベクトル
        Vector3 Reflect(const Vector3& normal) const {
            return *this - normal * (2.0f * Dot(normal));
        }
    };

    /**
     * @brief 4Dベクトル（同次座標）
     */
    struct Vector4 {
        float x, y, z, w;

        Vector4() : x(0), y(0), z(0), w(0) {}
        Vector4(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}
        Vector4(const Vector3& v, float w) : x(v.x), y(v.y), z(v.z), w(w) {}

        Vector3 ToVector3() const { return Vector3(x, y, z); }
    };

    /**
     * @brief 4x4行列
     *
     * メモリレイアウト：行優先（Row-major）
     * m[行][列]
     *
     * DirectX 12では、C++側は行優先、HLSL側は列優先として扱う。
     * そのため、シェーダーに渡す前に転置が必要な場合がある。
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

        // 単位行列を返す静的メソッド
        static Matrix4x4 CreateIdentity() {
            Matrix4x4 mat;
            mat.Identity();
            return mat;
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

        // ベクトルとの乗算
        Vector4 operator*(const Vector4& v) const {
            Vector4 result;
            result.x = m[0][0] * v.x + m[0][1] * v.y + m[0][2] * v.z + m[0][3] * v.w;
            result.y = m[1][0] * v.x + m[1][1] * v.y + m[1][2] * v.z + m[1][3] * v.w;
            result.z = m[2][0] * v.x + m[2][1] * v.y + m[2][2] * v.z + m[2][3] * v.w;
            result.w = m[3][0] * v.x + m[3][1] * v.y + m[3][2] * v.z + m[3][3] * v.w;
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

        // HLSL定数バッファ用転置（一貫性のため）
        Matrix4x4 ToHLSL() const {
            return Transpose();
        }

        // 行列式（3x3余因子）
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

        // 行列式
        float Determinant() const {
            float det = 0.0f;
            for (int i = 0; i < 4; ++i) {
                float sign = (i % 2 == 0) ? 1.0f : -1.0f;
                det += sign * m[0][i] * Determinant3x3(0, i);
            }
            return det;
        }

        // 逆行列
        Matrix4x4 Inverse() const {
            Matrix4x4 result;
            float det = Determinant();

            if (std::abs(det) < 1e-6f) {
                // 逆行列が存在しない場合は単位行列を返す
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

        // 平行移動行列
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

        // スケール行列
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

        // 任意軸回転行列（Rodriguesの回転公式）
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

        // ビュー行列（右手座標系）
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

        // ビュー行列（左手座標系）
        static Matrix4x4 LookAtRH(const Vector3& eye, const Vector3& target, const Vector3& up) {
            Vector3 zAxis = (eye - target).Normalize();  // 前方向（右手系は逆）
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

        // デフォルトのLookAt（右手座標系）
        static Matrix4x4 LookAt(const Vector3& eye, const Vector3& target, const Vector3& up) {
            return LookAtLH(eye, target, up);
        }

        // 透視投影行列（右手座標系）
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

        // 透視投影行列（左手座標系）
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

        // デフォルトのPerspective（右手座標系）
        static Matrix4x4 Perspective(float fovY, float aspect, float nearZ, float farZ) {
            return PerspectiveFovLH(fovY, aspect, nearZ, farZ);
        }

        // 正射影行列（右手座標系）
        static Matrix4x4 OrthographicLH(float width, float height, float nearZ, float farZ) {
            Matrix4x4 mat;
            mat.m[0][0] = 2.0f / width;
            mat.m[1][1] = 2.0f / height;
            mat.m[2][2] = 1.0f / (farZ - nearZ);
            mat.m[3][2] = -nearZ / (farZ - nearZ);
            return mat;
        }

        // 正射影行列（左手座標系）
        static Matrix4x4 OrthographicRH(float width, float height, float nearZ, float farZ) {
            Matrix4x4 mat;
            mat.m[0][0] = 2.0f / width;
            mat.m[1][1] = 2.0f / height;
            mat.m[2][2] = 1.0f / (nearZ - farZ);
            mat.m[3][2] = nearZ / (nearZ - farZ);
            return mat;
        }

        // オフセンター正射影（右手座標系）
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

        // オフセンター正射影（左手座標系）
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

    // 定数
    constexpr float PI = 3.14159265358979323846f;
    constexpr float TWO_PI = 2.0f * PI;
    constexpr float HALF_PI = 0.5f * PI;
    constexpr float EPSILON = 1e-6f;

    // 度をラジアンに変換
    inline float ToRadians(float degrees) {
        return degrees * (PI / 180.0f);
    }

    // ラジアンを度に変換
    inline float ToDegrees(float radians) {
        return radians * (180.0f / PI);
    }

    // 値のクランプ
    inline float Clamp(float value, float minVal, float maxVal) {
        return (std::max)(minVal, (std::min)(value, maxVal));
    }

    // 線形補間
    inline float Lerp(float a, float b, float t) {
        return a + (b - a) * t;
    }

    // 滑らか補間（Smoothstep）
    inline float Smoothstep(float edge0, float edge1, float x) {
        float t = Clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
        return t * t * (3.0f - 2.0f * t);
    }

} // namespace Athena