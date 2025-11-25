// Mesh.h - ���b�V���N���X
#pragma once

#include "Athena/Utils/Math.h"
#include "Athena/Resources/Buffer.h"
#include <vector>
#include <memory>

namespace Athena {

    // =====================================================
    // ���_�t�H�[�}�b�g
    // =====================================================

    /**
     * @brief �W���I�Ȓ��_�\��
     *
     * - Position: ���_��3D���W�ix, y, z�j
     * - Normal: �@���x�N�g�� - �ʂ̌����������i���C�e�B���O�v�Z�Ɏg���j
     * - TexCoord: �e�N�X�`�����W�iUV���W�j- �摜�̂ǂ̕�����\�邩
     */
    struct StandardVertex {
        Vector3 position;   // �ʒu�i12�o�C�g�j
        Vector3 normal;     // �@���i12�o�C�g�j
        float u, v;         // �e�N�X�`�����W�i8�o�C�g�j

        StandardVertex()
            : position(0, 0, 0)
            , normal(0, 1, 0)
            , u(0), v(0)
        {
        }

        StandardVertex(const Vector3& pos, const Vector3& norm, float u, float v)
            : position(pos)
            , normal(norm)
            , u(u), v(v)
        {
        }

        // �ȈՔŁi�@���͌�Ōv�Z����p�j
        StandardVertex(const Vector3& pos, float u, float v)
            : position(pos)
            , normal(0, 1, 0)
            , u(u), v(v)
        {
        }
    };

    // =====================================================
    // �T�u���b�V��
    // =====================================================

    /**
     * @brief �T�u���b�V��
     *
     * �T�u���b�V�� = ���b�V���̈ꕔ��
     * ��F�Ԃ̃��f��
     *   - �T�u���b�V��0: �{�f�B�i�Ԃ��}�e���A���j
     *   - �T�u���b�V��1: �^�C���i�����}�e���A���j
     *
     * �قȂ�}�e���A�����g�������������Ƃɕ�������
     */
    struct SubMesh {
        uint32_t indexStart;   // �C���f�b�N�X�o�b�t�@�̊J�n�ʒu
        uint32_t indexCount;   // �g�p����C���f�b�N�X��
        uint32_t materialID;   // �ǂ̃}�e���A�����g�����i��Ŏ����j

        SubMesh()
            : indexStart(0)
            , indexCount(0)
            , materialID(0)
        {
        }

        SubMesh(uint32_t start, uint32_t count, uint32_t matID = 0)
            : indexStart(start)
            , indexCount(count)
            , materialID(matID)
        {
        }
    };

    // =====================================================
    // ���b�V���N���X
    // =====================================================

    /**
     * @brief ���b�V���N���X
     *
     * 3D���f���̌`��f�[�^���Ǘ�����
     * - ���_�f�[�^
     * - �C���f�b�N�X�f�[�^
     * - �T�u���b�V�����
     */
    class Mesh {
    public:
        Mesh();
        ~Mesh();

        // ===== ������ =====

        /**
         * @brief CPU�������Ƀf�[�^��ݒ�
         *
         * @param vertices ���_�f�[�^�̔z��
         * @param vertexCount ���_��
         * @param indices �C���f�b�N�X�f�[�^�̔z��
         * @param indexCount �C���f�b�N�X��
         */
        void SetVertices(const StandardVertex* vertices, uint32_t vertexCount);
        void SetIndices(const uint32_t* indices, uint32_t indexCount);

        /**
         * @brief GPU�o�b�t�@���쐬���ăf�[�^���A�b�v���[�h
         */
        void UploadToGPU(ID3D12Device* device);

        /**
         * @brief �T�u���b�V����ǉ�
         */
        void AddSubMesh(const SubMesh& subMesh);

        // ===== �`�� =====

        /**
         * @brief �`��R�}���h�𔭍s
         *
         * @param commandList �`��R�}���h���L�^���郊�X�g
         * @param subMeshIndex �ǂ̃T�u���b�V����`�悷�邩�i-1 = �S���j
         */
        void Draw(ID3D12GraphicsCommandList* commandList, int subMeshIndex = -1);

        // ===== ���擾 =====

        uint32_t GetVertexCount() const { return static_cast<uint32_t>(vertices.size()); }
        uint32_t GetIndexCount() const { return static_cast<uint32_t>(indices.size()); }
        uint32_t GetSubMeshCount() const { return static_cast<uint32_t>(subMeshes.size()); }

        const std::vector<StandardVertex>& GetVertices() const { return vertices; }
        const std::vector<uint32_t>& GetIndices() const { return indices; }

        // ===== �o�b�t�@�A�N�Z�X =====
        Buffer* GetVertexBuffer() { return vertexBuffer.get(); }
        Buffer* GetIndexBuffer() { return indexBuffer.get(); }
        
        // ===== Material =====
        uint32_t GetMaterialIndex() const { return materialIndex; }
        void SetMaterialIndex(uint32_t index) { materialIndex = index; }
        
        // ===== Model Loading Support =====
        void SetVertexData(const void* data, size_t dataSize, size_t stride);
        void SetIndexData(const void* data, size_t dataSize, size_t indexCount);
        void CreateBuffers(ID3D12Device* device);
        
        // Raw data access methods for RenderGraph integration
        size_t GetRawVertexDataSize() const { return rawVertexData.size(); }
        size_t GetRawIndexDataSize() const { return rawIndexData.size(); }
        size_t GetVertexStride() const { return vertexStride; }
        size_t GetRawIndexCount() const { return rawIndexCount; }
        const void* GetRawVertexData() const { return rawVertexData.data(); }
        const void* GetRawIndexData() const { return rawIndexData.data(); }

        // ===== ���[�e�B���e�B =====

        /**
         * @brief �@���x�N�g���������v�Z
         *
         * �@�� = �ʂ̌����������x�N�g��
         * ���C�e�B���O�i���̌v�Z�j�ɕK�{
         * �O�p�`��2�ӂ̊O�ςŌv�Z�ł���
         */
        void CalculateNormals();

        /**
         * @brief �o�E���f�B���O�{�b�N�X���v�Z
         *
         * �o�E���f�B���O�{�b�N�X = ���b�V�����͂ޒ�����
         * �Փ˔����J�����O�i��ʊO����j�Ɏg��
         */
        void CalculateBounds();

        Vector3 GetBoundsMin() const { return boundsMin; }
        Vector3 GetBoundsMax() const { return boundsMax; }
        Vector3 GetBoundsCenter() const { return (boundsMin + boundsMax) * 0.5f; }
        Vector3 GetBoundsSize() const { return boundsMax - boundsMin; }

    private:
        // CPU�������̃f�[�^
        std::vector<StandardVertex> vertices;
        std::vector<uint32_t> indices;
        std::vector<SubMesh> subMeshes;

        // GPU�o�b�t�@
        std::unique_ptr<Buffer> vertexBuffer;
        std::unique_ptr<Buffer> indexBuffer;

        // Bounding box
        Vector3 boundsMin;
        Vector3 boundsMax;
        
        // Material index for ModelLoader compatibility  
        uint32_t materialIndex = 0;
        
        // Vertex/Index counts for ModelLoader
        uint32_t vertexCount = 0;
        uint32_t indexCount = 0;
        
        // Raw data storage for model loading
        std::vector<uint8_t> rawVertexData;
        std::vector<uint8_t> rawIndexData;
        size_t vertexStride = 0;
        size_t rawIndexCount = 0;

        // ���_���C�A�E�g�i���_�̍\����GPU�ɋ�����j
        static constexpr uint32_t VERTEX_STRIDE = sizeof(StandardVertex);  // 32�o�C�g
    };

    // =====================================================
    // �v���~�e�B�u�����i�֗��֐��j
    // =====================================================

    /**
     * @brief ��{�I�Ȍ`��𐶐�����w���p�[�֐�
     */
    namespace MeshGenerator {

        /**
         * @brief �L���[�u�i�����́j�𐶐�
         */
        std::unique_ptr<Mesh> CreateCube(float size = 1.0f);

        /**
         * @brief ���̂𐶐�
         *
         * @param radius ���a
         * @param slices �o�x�����̕������i���j
         * @param stacks �ܓx�����̕������i�c�j
         */
        std::unique_ptr<Mesh> CreateSphere(float radius = 1.0f, uint32_t slices = 32, uint32_t stacks = 16);

        /**
         * @brief ���ʂ𐶐�
         */
        std::unique_ptr<Mesh> CreatePlane(float width = 1.0f, float depth = 1.0f, uint32_t subdivisions = 1);

    } // namespace MeshGenerator

} // namespace Athena