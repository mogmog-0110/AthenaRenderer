#pragma once

#include <d3d12.h>
#include <wrl/client.h>
#include <cstdint>

namespace Athena {

    using Microsoft::WRL::ComPtr;

    /**
     * @brief �o�b�t�@�̎��
     */
    enum class BufferType {
        Vertex,      // ���_�o�b�t�@
        Index,       // �C���f�b�N�X�o�b�t�@
        Constant,    // �萔�o�b�t�@
        Structured   // �\�����o�b�t�@
    };

    /**
     * @brief GPU �o�b�t�@�̒��ۉ��N���X
     *
     * DirectX 12�̗l�X�ȃo�b�t�@�i���_�A�C���f�b�N�X�A�萔�Ȃǁj��
     * ����I�Ɉ������߂̊��N���X
     */
    class Buffer {
    public:
        Buffer();
        ~Buffer();

        /**
         * @brief �o�b�t�@�̏�����
         * @param device DirectX 12�f�o�C�X
         * @param size �o�b�t�@�T�C�Y�i�o�C�g�j
         * @param type �o�b�t�@�̎��
         * @param heapType �q�[�v�^�C�v�iDEFAULT or UPLOAD�j
         */
        void Initialize(
            ID3D12Device* device,
            uint64_t size,
            BufferType type,
            D3D12_HEAP_TYPE heapType = D3D12_HEAP_TYPE_DEFAULT
        );

        /**
         * @brief �o�b�t�@�̏I������
         */
        void Shutdown();

        /**
         * @brief �f�[�^���o�b�t�@�ɃA�b�v���[�h
         * @param data �A�b�v���[�h����f�[�^
         * @param size �f�[�^�T�C�Y�i�o�C�g�j
         * @param offset �o�b�t�@���̃I�t�Z�b�g
         */
        void Upload(const void* data, uint64_t size, uint64_t offset = 0);

        /**
         * @brief DEFAULTヒープ用のアップロード（デバイスとコマンドキュー指定）
         */
        void UploadWithDevice(const void* data, uint64_t size, 
                            ID3D12Device* device, ID3D12CommandQueue* commandQueue,
                            uint64_t offset = 0);

        /**
         * @brief �o�b�t�@���}�b�v�iCPU�A�N�Z�X�\�ɂ���j
         * @return �}�b�v���ꂽ�������ւ̃|�C���^
         */
        void* Map();

        /**
         * @brief �o�b�t�@���A���}�b�v
         */
        void Unmap();

        // �A�N�Z�T
        ID3D12Resource* GetResource() const { return resource.Get(); }
        ID3D12Resource* GetD3D12Resource() const { return resource.Get(); }  // RenderGraphバリア用
        D3D12_GPU_VIRTUAL_ADDRESS GetGPUVirtualAddress() const;
        uint64_t GetSize() const { return size; }
        BufferType GetType() const { return type; }

        // �r���[�擾
        D3D12_VERTEX_BUFFER_VIEW GetVertexBufferView() const;
        D3D12_INDEX_BUFFER_VIEW GetIndexBufferView() const;
        D3D12_CONSTANT_BUFFER_VIEW_DESC GetConstantBufferViewDesc() const;

    protected:
        ComPtr<ID3D12Resource> resource;
        uint64_t size = 0;
        BufferType type;
        D3D12_HEAP_TYPE heapType;
        void* mappedData = nullptr;

    private:
        void CreateResource(ID3D12Device* device);
    };

} // namespace Athena