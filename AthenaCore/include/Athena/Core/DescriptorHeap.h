#pragma once

#include <d3d12.h>
#include <wrl/client.h>
#include <cstdint>
#include <vector>

namespace Athena {

    using Microsoft::WRL::ComPtr;

    /**
     * @brief �f�X�N���v�^�n���h��
     *
     * CPU�n���h����GPU�n���h���̗�����ێ��B
     * �f�X�N���v�^�̓��\�[�X�i�e�N�X�`���A�o�b�t�@���j�ւ̎Q�ƁB
     */
    struct DescriptorHandle {
        D3D12_CPU_DESCRIPTOR_HANDLE cpu;  // CPU���n���h��
        D3D12_GPU_DESCRIPTOR_HANDLE gpu;  // GPU���n���h���i�V�F�[�_�[���̏ꍇ�̂݁j
        uint32_t index = 0;                // �q�[�v���̃C���f�b�N�X

        bool IsValid() const { return cpu.ptr != 0; }
        bool IsShaderVisible() const { return gpu.ptr != 0; }
    };

    /**
     * @brief �f�X�N���v�^�q�[�v�Ǘ��N���X
     *
     * �f�X�N���v�^�i���\�[�X�L�q�q�j���Ǘ��B
     * DirectX 12�ł́A���\�[�X�ւ̃A�N�Z�X�Ƀf�X�N���v�^���K�v�B
     *
     * �f�X�N���v�^�q�[�v�̎�ށF
     * - RTV (Render Target View): �����_�[�^�[�Q�b�g�p
     * - DSV (Depth Stencil View): �[�x�o�b�t�@�p
     * - CBV/SRV/UAV: �萔�o�b�t�@�A�e�N�X�`���AUAV�p
     * - Sampler: �T���v���[�p
     */
    class DescriptorHeap {
    public:
        DescriptorHeap();
        ~DescriptorHeap();

        /**
         * @brief �f�X�N���v�^�q�[�v��������
         *
         * @param device D3D12�f�o�C�X
         * @param type �q�[�v�̃^�C�v�iRTV, DSV, CBV_SRV_UAV, Sampler�j
         * @param capacity �q�[�v�̗e�ʁi�f�X�N���v�^���j
         * @param shaderVisible �V�F�[�_�[����Q�Ɖ\�ɂ��邩
         *
         * shader Visible��true�ɂł���̂́ACBV_SRV_UAV��Sampler�̂݁B
         * RTV��DSV�͏�ɃV�F�[�_�[�s���ł��B
         */
        void Initialize(
            ID3D12Device* device,
            D3D12_DESCRIPTOR_HEAP_TYPE type,
            uint32_t capacity,
            bool shaderVisible = false
        );

        /**
         * @brief �f�X�N���v�^�q�[�v���V���b�g�_�E��
         */
        void Shutdown();

        /**
         * @brief �f�X�N���v�^�����蓖��
         *
         * @return ���蓖�Ă�ꂽ�f�X�N���v�^�n���h��
         *
         * �q�[�v�����t�̏ꍇ�͗�O���X���[�B
         */
        DescriptorHandle Allocate();

        /**
         * @brief �f�X�N���v�^�����
         *
         * @param handle �������n���h��
         *
         * ������ꂽ�f�X�N���v�^�͍ė��p�\�ɂȂ�B
         */
        void Free(const DescriptorHandle& handle);

        /**
         * @brief D3D12�f�X�N���v�^�q�[�v�ւ̃A�N�Z�X
         */
        ID3D12DescriptorHeap* GetD3D12DescriptorHeap() const { return heap.Get(); }

        /**
         * @brief �f�X�N���v�^�̃T�C�Y���擾
         *
         * @return �f�X�N���v�^1���̃o�C�g��
         *
         * ���̃T�C�Y��GPU�ɂ���ĈقȂ�B
         */
        uint32_t GetDescriptorSize() const { return descriptorSize; }

    private:
        ComPtr<ID3D12DescriptorHeap> heap;      // �q�[�v�{��
        D3D12_DESCRIPTOR_HEAP_TYPE type;        // �q�[�v�̃^�C�v
        uint32_t capacity = 0;                  // �e��
        uint32_t descriptorSize = 0;            // �f�X�N���v�^�̃T�C�Y
        bool shaderVisible = false;             // �V�F�[�_�[����

        D3D12_CPU_DESCRIPTOR_HANDLE cpuStart = {};  // CPU���̊J�n�n���h��
        D3D12_GPU_DESCRIPTOR_HANDLE gpuStart = {};  // GPU���̊J�n�n���h��

        std::vector<bool> allocated;            // ���蓖�ď�
        uint32_t nextFreeIndex = 0;             // ���̃t���[�ȃC���f�b�N�X
    };

} // namespace Athena