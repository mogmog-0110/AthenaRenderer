#pragma once

#include <d3d12.h>
#include <wrl/client.h>
#include <cstdint>
#include <vector>
#include <functional>

namespace Athena {

    using Microsoft::WRL::ComPtr;

    /**
     * @brief �A�b�v���[�h�R���e�L�X�g
     *
     * �e�N�X�`����o�b�t�@��GPU�ɃA�b�v���[�h���邽�߂�
     * �ꎞ�I�ȃR�}���h���X�g�ƃ��\�[�X�Ǘ����s��
     */
    class UploadContext {
    public:
        UploadContext();
        ~UploadContext();

        /**
         * @brief ������
         * @param device DirectX 12�f�o�C�X
         * @param commandQueue �R�}���h�L���[
         */
        void Initialize(ID3D12Device* device, ID3D12CommandQueue* commandQueue);

        /**
         * @brief �I������
         */
        void Shutdown();

        /**
         * @brief �A�b�v���[�h���J�n
         * �A�b�v���[�h�p�R�}���h���X�g���J�n��Ԃɂ���
         */
        void Begin();

        /**
         * @brief �o�b�t�@�f�[�^���A�b�v���[�h
         * @param destinationResource �]���惊�\�[�X
         * @param data �A�b�v���[�h����f�[�^
         * @param dataSize �f�[�^�T�C�Y
         */
        void UploadBuffer(
            ID3D12Resource* destinationResource,
            const void* data,
            uint64_t dataSize
        );

        /**
         * @brief �e�N�X�`���f�[�^���A�b�v���[�h
         * @param destinationResource �]����e�N�X�`�����\�[�X
         * @param subresourceData �T�u���\�[�X�f�[�^
         * @param numSubresources �T�u���\�[�X��
         */
        void UploadTexture(
            ID3D12Resource* destinationResource,
            const D3D12_SUBRESOURCE_DATA* subresourceData,
            uint32_t numSubresources
        );

        /**
         * @brief ���\�[�X�o���A��ǉ�
         * @param resource �Ώۃ��\�[�X
         * @param stateBefore �ύX�O�̏��
         * @param stateAfter �ύX��̏��
         */
        void TransitionResource(
            ID3D12Resource* resource,
            D3D12_RESOURCE_STATES stateBefore,
            D3D12_RESOURCE_STATES stateAfter
        );

        /**
         * @brief �A�b�v���[�h���I������GPU�ɑ��M
         * �R�}���h���X�g����Ď��s���A������ҋ@����
         */
        void End();

        /**
         * @brief �R�}���h���X�g�ւ̃A�N�Z�X
         */
        ID3D12GraphicsCommandList* GetCommandList() const { return commandList.Get(); }

    private:
        ComPtr<ID3D12Device> device;
        ComPtr<ID3D12CommandQueue> commandQueue;
        ComPtr<ID3D12CommandAllocator> commandAllocator;
        ComPtr<ID3D12GraphicsCommandList> commandList;

        ComPtr<ID3D12Fence> fence;
        HANDLE fenceEvent = nullptr;
        uint64_t fenceValue = 0;

        // �ꎞ�I�ȃA�b�v���[�h�o�b�t�@�iEnd()�Ŏ����폜�j
        std::vector<ComPtr<ID3D12Resource>> uploadBuffers;

        void WaitForGPU();
        void ResetCommandList();
    };

} // namespace Athena