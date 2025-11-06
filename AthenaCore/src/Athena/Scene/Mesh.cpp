// Mesh.cpp - メッシュクラスの実装
#define NOMINMAX
#include "Athena/Scene/Mesh.h"
#include "Athena/Utils/Logger.h"
#include <algorithm>
#include <limits>

namespace Athena {

    // =====================================================
    // Mesh クラス
    // =====================================================

    Mesh::Mesh()
        : boundsMin(0, 0, 0)
        , boundsMax(0, 0, 0)
    {
    }

    Mesh::~Mesh() = default;

    void Mesh::SetVertices(const StandardVertex* vertices, uint32_t vertexCount) {
        this->vertices.assign(vertices, vertices + vertexCount);
        Logger::Info("Mesh: Set %u vertices", vertexCount);
    }

    void Mesh::SetIndices(const uint32_t* indices, uint32_t indexCount) {
        this->indices.assign(indices, indices + indexCount);
        Logger::Info("Mesh: Set %u indices (%u triangles)", indexCount, indexCount / 3);
    }

    void Mesh::UploadToGPU(ID3D12Device* device) {
        if (vertices.empty() || indices.empty()) {
            Logger::Warning("Mesh: Cannot upload empty mesh to GPU");
            return;
        }

        // 頂点バッファ作成
        vertexBuffer = std::make_unique<Buffer>();
        vertexBuffer->Initialize(
            device,
            static_cast<uint32_t>(vertices.size() * sizeof(StandardVertex)),
            BufferType::Vertex,
            D3D12_HEAP_TYPE_UPLOAD
        );
        vertexBuffer->Upload(vertices.data(), static_cast<uint32_t>(vertices.size() * sizeof(StandardVertex)));

        // インデックスバッファ作成
        indexBuffer = std::make_unique<Buffer>();
        indexBuffer->Initialize(
            device,
            static_cast<uint32_t>(indices.size() * sizeof(uint32_t)),
            BufferType::Index,
            D3D12_HEAP_TYPE_UPLOAD
        );
        indexBuffer->Upload(indices.data(), static_cast<uint32_t>(indices.size() * sizeof(uint32_t)));

        Logger::Info("✓ Mesh uploaded to GPU");
    }

    void Mesh::AddSubMesh(const SubMesh& subMesh) {
        subMeshes.push_back(subMesh);
        Logger::Info("Mesh: Added submesh (indices: %u - %u)",
            subMesh.indexStart, subMesh.indexStart + subMesh.indexCount);
    }

    void Mesh::Draw(ID3D12GraphicsCommandList* commandList, int subMeshIndex) {
        if (!vertexBuffer || !indexBuffer) {
            Logger::Warning("Mesh: Cannot draw - buffers not uploaded");
            return;
        }

        // 頂点バッファ設定
        D3D12_VERTEX_BUFFER_VIEW vbv = vertexBuffer->GetVertexBufferView();
        vbv.StrideInBytes = VERTEX_STRIDE;
        commandList->IASetVertexBuffers(0, 1, &vbv);

        // インデックスバッファ設定
        D3D12_INDEX_BUFFER_VIEW ibv = indexBuffer->GetIndexBufferView();
        commandList->IASetIndexBuffer(&ibv);

        // プリミティブトポロジー設定
        commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        // 描画
        if (subMeshIndex < 0 && subMeshes.empty()) {
            // サブメッシュなし → 全体を描画
            commandList->DrawIndexedInstanced(
                static_cast<uint32_t>(indices.size()),  // インデックス数
                1,                                       // インスタンス数
                0,                                       // 開始インデックス
                0,                                       // 頂点オフセット
                0                                        // 開始インスタンス
            );
        }
        else if (subMeshIndex < 0) {
            // 全サブメッシュを描画
            for (const auto& subMesh : subMeshes) {
                commandList->DrawIndexedInstanced(
                    subMesh.indexCount,
                    1,
                    subMesh.indexStart,
                    0,
                    0
                );
            }
        }
        else if (subMeshIndex < static_cast<int>(subMeshes.size())) {
            // 指定されたサブメッシュのみ描画
            const auto& subMesh = subMeshes[subMeshIndex];
            commandList->DrawIndexedInstanced(
                subMesh.indexCount,
                1,
                subMesh.indexStart,
                0,
                0
            );
        }
    }

    void Mesh::CalculateNormals() {
        if (vertices.empty() || indices.empty()) {
            Logger::Warning("Mesh: Cannot calculate normals - no data");
            return;
        }

        // まず全頂点の法線をゼロクリア
        for (auto& vertex : vertices) {
            vertex.normal = Vector3(0, 0, 0);
        }

        // 各三角形の法線を計算して、頂点に加算
        for (size_t i = 0; i < indices.size(); i += 3) {
            uint32_t i0 = indices[i];
            uint32_t i1 = indices[i + 1];
            uint32_t i2 = indices[i + 2];

            // 三角形の3頂点
            Vector3 v0 = vertices[i0].position;
            Vector3 v1 = vertices[i1].position;
            Vector3 v2 = vertices[i2].position;

            // 【法線計算の原理】
            // 三角形の2辺のベクトルを作る
            Vector3 edge1 = v1 - v0;
            Vector3 edge2 = v2 - v0;

            // 外積を取ると、面に垂直なベクトル（法線）が得られる
            // 外積の方向 = 右手の法則
            Vector3 faceNormal = edge1.Cross(edge2);

            // この面の法線を、3頂点すべてに加算
            vertices[i0].normal += faceNormal;
            vertices[i1].normal += faceNormal;
            vertices[i2].normal += faceNormal;
        }

        // 全頂点の法線を正規化（長さを1にする）
        for (auto& vertex : vertices) {
            vertex.normal = vertex.normal.Normalize();
        }

        Logger::Info("✓ Mesh: Normals calculated");
    }

    void Mesh::CalculateBounds() {
        if (vertices.empty()) {
            boundsMin = Vector3(0, 0, 0);
            boundsMax = Vector3(0, 0, 0);
            return;
        }

        // 初期値を設定（浮動小数点の最大/最小値）
        boundsMin = Vector3(
            std::numeric_limits<float>::max(),
            std::numeric_limits<float>::max(),
            std::numeric_limits<float>::max()
        );
        boundsMax = Vector3(
            std::numeric_limits<float>::lowest(),
            std::numeric_limits<float>::lowest(),
            std::numeric_limits<float>::lowest()
        );

        // 全頂点をチェックして、最小値・最大値を更新
        for (const auto& vertex : vertices) {
            boundsMin.x = std::min(boundsMin.x, vertex.position.x);
            boundsMin.y = std::min(boundsMin.y, vertex.position.y);
            boundsMin.z = std::min(boundsMin.z, vertex.position.z);

            boundsMax.x = std::max(boundsMax.x, vertex.position.x);
            boundsMax.y = std::max(boundsMax.y, vertex.position.y);
            boundsMax.z = std::max(boundsMax.z, vertex.position.z);
        }

        Logger::Info("✓ Mesh: Bounds calculated - Min(%.2f, %.2f, %.2f) Max(%.2f, %.2f, %.2f)",
            boundsMin.x, boundsMin.y, boundsMin.z,
            boundsMax.x, boundsMax.y, boundsMax.z);
    }

    // =====================================================
    // MeshGenerator - プリミティブ生成
    // =====================================================

    namespace MeshGenerator {

        std::unique_ptr<Mesh> CreateCube(float size) {
            auto mesh = std::make_unique<Mesh>();

            float halfSize = size * 0.5f;

            // 【キューブの頂点構造】
            // キューブは6面 × 4頂点 = 24頂点必要
            // （各面で独立した法線・UV座標を持つため）

            std::vector<StandardVertex> vertices = {
                // 前面（Z+）
                {{-halfSize, -halfSize,  halfSize}, {0, 0, 1}, 0, 1},
                {{-halfSize,  halfSize,  halfSize}, {0, 0, 1}, 0, 0},
                {{ halfSize,  halfSize,  halfSize}, {0, 0, 1}, 1, 0},
                {{ halfSize, -halfSize,  halfSize}, {0, 0, 1}, 1, 1},

                // 背面（Z-）
                {{ halfSize, -halfSize, -halfSize}, {0, 0, -1}, 0, 1},
                {{ halfSize,  halfSize, -halfSize}, {0, 0, -1}, 0, 0},
                {{-halfSize,  halfSize, -halfSize}, {0, 0, -1}, 1, 0},
                {{-halfSize, -halfSize, -halfSize}, {0, 0, -1}, 1, 1},

                // 上面（Y+）
                {{-halfSize,  halfSize,  halfSize}, {0, 1, 0}, 0, 1},
                {{-halfSize,  halfSize, -halfSize}, {0, 1, 0}, 0, 0},
                {{ halfSize,  halfSize, -halfSize}, {0, 1, 0}, 1, 0},
                {{ halfSize,  halfSize,  halfSize}, {0, 1, 0}, 1, 1},

                // 底面（Y-）
                {{-halfSize, -halfSize, -halfSize}, {0, -1, 0}, 0, 1},
                {{-halfSize, -halfSize,  halfSize}, {0, -1, 0}, 0, 0},
                {{ halfSize, -halfSize,  halfSize}, {0, -1, 0}, 1, 0},
                {{ halfSize, -halfSize, -halfSize}, {0, -1, 0}, 1, 1},

                // 右面（X+）
                {{ halfSize, -halfSize,  halfSize}, {1, 0, 0}, 0, 1},
                {{ halfSize,  halfSize,  halfSize}, {1, 0, 0}, 0, 0},
                {{ halfSize,  halfSize, -halfSize}, {1, 0, 0}, 1, 0},
                {{ halfSize, -halfSize, -halfSize}, {1, 0, 0}, 1, 1},

                // 左面（X-）
                {{-halfSize, -halfSize, -halfSize}, {-1, 0, 0}, 0, 1},
                {{-halfSize,  halfSize, -halfSize}, {-1, 0, 0}, 0, 0},
                {{-halfSize,  halfSize,  halfSize}, {-1, 0, 0}, 1, 0},
                {{-halfSize, -halfSize,  halfSize}, {-1, 0, 0}, 1, 1},
            };

            // インデックス（各面を2つの三角形で構成）
            std::vector<uint32_t> indices = {
                0, 1, 2,   0, 2, 3,    // 前面
                4, 5, 6,   4, 6, 7,    // 背面
                8, 9, 10,  8, 10, 11,  // 上面
                12, 13, 14, 12, 14, 15, // 底面
                16, 17, 18, 16, 18, 19, // 右面
                20, 21, 22, 20, 22, 23, // 左面
            };

            mesh->SetVertices(vertices.data(), static_cast<uint32_t>(vertices.size()));
            mesh->SetIndices(indices.data(), static_cast<uint32_t>(indices.size()));
            mesh->CalculateBounds();

            Logger::Info("✓ Created cube mesh (size: %.2f)", size);
            return mesh;
        }

        std::unique_ptr<Mesh> CreateSphere(float radius, uint32_t slices, uint32_t stacks) {
            auto mesh = std::make_unique<Mesh>();

            std::vector<StandardVertex> vertices;
            std::vector<uint32_t> indices;

            // 【球体生成の原理】
            // 緯度（latitude）と経度（longitude）で分割
            // 極座標 → 直交座標への変換

            // 頂点生成
            for (uint32_t stack = 0; stack <= stacks; ++stack) {
                float phi = static_cast<float>(stack) / stacks * PI;  // 0〜π（縦方向）
                float sinPhi = std::sin(phi);
                float cosPhi = std::cos(phi);

                for (uint32_t slice = 0; slice <= slices; ++slice) {
                    float theta = static_cast<float>(slice) / slices * TWO_PI;  // 0〜2π（横方向）
                    float sinTheta = std::sin(theta);
                    float cosTheta = std::cos(theta);

                    // 球面座標 → 直交座標
                    Vector3 pos(
                        radius * sinPhi * cosTheta,  // x
                        radius * cosPhi,             // y
                        radius * sinPhi * sinTheta   // z
                    );

                    // 球の法線 = 中心から頂点への方向
                    Vector3 normal = pos.Normalize();

                    // UV座標
                    float u = static_cast<float>(slice) / slices;
                    float v = static_cast<float>(stack) / stacks;

                    vertices.push_back(StandardVertex(pos, normal, u, v));
                }
            }

            // インデックス生成
            for (uint32_t stack = 0; stack < stacks; ++stack) {
                for (uint32_t slice = 0; slice < slices; ++slice) {
                    uint32_t i0 = stack * (slices + 1) + slice;
                    uint32_t i1 = i0 + 1;
                    uint32_t i2 = (stack + 1) * (slices + 1) + slice;
                    uint32_t i3 = i2 + 1;

                    // 四角形を2つの三角形に分割
                    indices.push_back(i0);
                    indices.push_back(i2);
                    indices.push_back(i1);

                    indices.push_back(i1);
                    indices.push_back(i2);
                    indices.push_back(i3);
                }
            }

            mesh->SetVertices(vertices.data(), static_cast<uint32_t>(vertices.size()));
            mesh->SetIndices(indices.data(), static_cast<uint32_t>(indices.size()));
            mesh->CalculateBounds();

            Logger::Info("✓ Created sphere mesh (radius: %.2f, slices: %u, stacks: %u)",
                radius, slices, stacks);
            return mesh;
        }

        std::unique_ptr<Mesh> CreatePlane(float width, float depth, uint32_t subdivisions) {
            auto mesh = std::make_unique<Mesh>();

            std::vector<StandardVertex> vertices;
            std::vector<uint32_t> indices;

            float halfWidth = width * 0.5f;
            float halfDepth = depth * 0.5f;

            // 頂点生成（グリッド状に配置）
            for (uint32_t z = 0; z <= subdivisions; ++z) {
                for (uint32_t x = 0; x <= subdivisions; ++x) {
                    float fx = static_cast<float>(x) / subdivisions;
                    float fz = static_cast<float>(z) / subdivisions;

                    Vector3 pos(
                        -halfWidth + fx * width,
                        0.0f,
                        -halfDepth + fz * depth
                    );

                    Vector3 normal(0, 1, 0);  // 上向き
                    float u = fx;
                    float v = fz;

                    vertices.push_back(StandardVertex(pos, normal, u, v));
                }
            }

            // インデックス生成
            for (uint32_t z = 0; z < subdivisions; ++z) {
                for (uint32_t x = 0; x < subdivisions; ++x) {
                    uint32_t i0 = z * (subdivisions + 1) + x;
                    uint32_t i1 = i0 + 1;
                    uint32_t i2 = (z + 1) * (subdivisions + 1) + x;
                    uint32_t i3 = i2 + 1;

                    indices.push_back(i0);
                    indices.push_back(i2);
                    indices.push_back(i1);

                    indices.push_back(i1);
                    indices.push_back(i2);
                    indices.push_back(i3);
                }
            }

            mesh->SetVertices(vertices.data(), static_cast<uint32_t>(vertices.size()));
            mesh->SetIndices(indices.data(), static_cast<uint32_t>(indices.size()));
            mesh->CalculateBounds();

            Logger::Info("✓ Created plane mesh (width: %.2f, depth: %.2f, subdivisions: %u)",
                width, depth, subdivisions);
            return mesh;
        }

    } // namespace MeshGenerator

} // namespace Athena