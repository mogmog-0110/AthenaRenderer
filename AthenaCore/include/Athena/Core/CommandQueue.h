#pragma once

#include <d3d12.h>
#include <wrl/client.h>
#include <cstdint>

namespace Athena {

    using Microsoft::WRL::ComPtr;

    /**
     * @brief �R�}���h�L���[�Ǘ��N���X
     *
     * GPU�ւ̃R�}���h���M�ƁACPU-GPU�Ԃ̓������Ǘ��B
     * DirectX 12�ł́A�R�}���h���X�g���R�}���h�L���[�ɑ��M���邱�Ƃ�
     * GPU�ɍ�Ƃ��w���B
     */
    class CommandQueue {
    public:
        CommandQueue();
        ~CommandQueue();

        /**
         * @brief �R�}���h�L���[��������
         *
         * @param device D3D12�f�o�C�X
         * @param type �R�}���h���X�g�̃^�C�v�iDirect/Compute/Copy�j
         *
         * DirectX 12�ł�3��ނ̃R�}���h�L���[�����݂���F
         * - Direct: �O���t�B�b�N�X�{�R���s���[�g�{�R�s�[�i�ł���ʓI�j
         * - Compute: �R���s���[�g�V�F�[�_�[��p
         * - Copy: �f�[�^�]����p
         */
        void Initialize(ID3D12Device* device, D3D12_COMMAND_LIST_TYPE type);

        /**
         * @brief �R�}���h�L���[���V���b�g�_�E��
         *
         * GPU�̊�����҂��Ă��烊�\�[�X������B
         */
        void Shutdown();

        /**
         * @brief �R�}���h���X�g��GPU�ɑ��M���Ď��s
         *
         * @param commandLists ���s����R�}���h���X�g�̔z��
         * @param count �R�}���h���X�g�̐�
         *
         * �R�}���h���X�g�͕K�� Close() ���ꂽ��Ԃœn���K�v������B
         */
        void ExecuteCommandLists(ID3D12CommandList* const* commandLists, uint32_t count);

        /**
         * @brief GPU�̍�Ɗ�����ҋ@
         *
         * CPU��GPU�̊�����҂K�v������ꍇ�Ɏg�p�B
         * ��F�t���[���I�����A���\�[�X����O�Ȃ�
         */
        void WaitForGPU();

        /**
         * @brief �t�F���X�ɃV�O�i���𑗂�
         *
         * GPU�����݂̃R�}���h�����������Ƃ��ɒʒm�����悤�ݒ�B
         */
        void Signal();

        /**
         * @brief GPU�����������t�F���X�l���擾
         *
         * @return ���������t�F���X�l
         */
        uint64_t GetCompletedValue() const;

        /**
         * @brief D3D12�R�}���h�L���[�ւ̃A�N�Z�X
         *
         * @return ID3D12CommandQueue �|�C���^
         */
        ID3D12CommandQueue* GetD3D12CommandQueue() const { return queue.Get(); }

    private:
        ComPtr<ID3D12CommandQueue> queue;      // �R�}���h�L���[�{��
        ComPtr<ID3D12Fence> fence;             // CPU-GPU�����p�t�F���X
        HANDLE fenceEvent = nullptr;           // �t�F���X�����ʒm�p�C�x���g
        uint64_t fenceValue = 0;               // ���݂̃t�F���X�l
    };

} // namespace Athena