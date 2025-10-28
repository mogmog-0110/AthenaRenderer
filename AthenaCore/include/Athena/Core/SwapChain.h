#pragma once

#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
#include <cstdint>
#include <vector>

namespace Athena {

    using Microsoft::WRL::ComPtr;

    /**
     * @brief �X���b�v�`�F�[���Ǘ��N���X
     *
     * ��ʂւ̕\�����Ǘ��B
     * �X���b�v�`�F�[���͕����̃o�b�N�o�b�t�@�������AGPU�ŕ`�悵�Ă���Ԃ�
     * �ʂ̃o�b�t�@����ʂɕ\�����邱�ƂŁA���炩�ȕ`�����������B
     */
    class SwapChain {
    public:
        static constexpr uint32_t BufferCount = 3; // �g���v���o�b�t�@�����O

        SwapChain();
        ~SwapChain();

        /**
         * @brief �X���b�v�`�F�[����������
         *
         * @param factory DXGI�t�@�N�g���[
         * @param commandQueue �R�}���h�L���[�iPresent�p�j
         * @param hwnd �E�B���h�E�n���h��
         * @param width ��ʕ�
         * @param height ��ʍ���
         *
         * �g���v���o�b�t�@�����O�i3���̃o�b�N�o�b�t�@�j���g�p�F
         * - Buffer 0: ���݉�ʂɕ\����
         * - Buffer 1: ���ɕ\�������i���������j
         * - Buffer 2: GPU���`�撆
         */
        void Initialize(
            IDXGIFactory6* factory,
            ID3D12CommandQueue* commandQueue,
            HWND hwnd,
            uint32_t width,
            uint32_t height
        );

        /**
         * @brief �X���b�v�`�F�[�����V���b�g�_�E��
         */
        void Shutdown();

        /**
         * @brief ��ʂɕ\���iPresent�j
         *
         * @param vsync ����������L���ɂ��邩
         *
         * vsync = true:  ��ʂ̃��t���b�V�����[�g�ɓ����i60Hz �Ȃ� 60FPS�j
         * vsync = false: �\�Ȍ��荂���ɕ`��i�e�B�A�����O����������\���j
         */
        void Present(bool vsync = true);

        /**
         * @brief ���݂̃o�b�N�o�b�t�@�C���f�b�N�X���擾
         *
         * @return �o�b�N�o�b�t�@�̃C���f�b�N�X�i0, 1, 2�̂����ꂩ�j
         *
         * Present() ���ĂԂ��тɃC���f�b�N�X���ς��B
         */
        uint32_t GetCurrentBackBufferIndex() const;

        /**
         * @brief ���݂̃o�b�N�o�b�t�@���擾
         *
         * @return ID3D12Resource �|�C���^
         *
         * ���̃o�b�t�@�ɕ`�悷�邱�ƂŁA���̃t���[���ŉ�ʂɕ\���B
         */
        ID3D12Resource* GetCurrentBackBuffer() const;

        /**
         * @brief �w�肵���C���f�b�N�X�̃o�b�N�o�b�t�@���擾
         *
         * @param index �o�b�N�o�b�t�@�̃C���f�b�N�X�i0, 1, 2�j
         * @return ID3D12Resource �|�C���^
         */
        ID3D12Resource* GetBackBuffer(uint32_t index) const;

        /**
         * @brief ��ʃT�C�Y��ύX
         *
         * @param width �V������
         * @param height �V��������
         *
         * �E�B���h�E�T�C�Y���ύX���ꂽ�Ƃ��ɌĂяo���B
         * �o�b�N�o�b�t�@����x������Ă���č쐬�B
         */
        void Resize(uint32_t width, uint32_t height);

        /**
         * @brief DXGI�X���b�v�`�F�[���ւ̃A�N�Z�X
         */
        IDXGISwapChain4* GetDXGISwapChain() const { return swapChain.Get(); }

        /**
         * @brief ��ʕ����擾
         */
        uint32_t GetWidth() const { return width; }

        /**
         * @brief ��ʍ������擾
         */
        uint32_t GetHeight() const { return height; }

        /**
         * @brief �o�b�N�o�b�t�@�̃t�H�[�}�b�g���擾
         */
        DXGI_FORMAT GetFormat() const { return format; }

    private:
        ComPtr<IDXGISwapChain4> swapChain;                      // �X���b�v�`�F�[���{��
        std::vector<ComPtr<ID3D12Resource>> backBuffers;        // �o�b�N�o�b�t�@�z��

        uint32_t width = 0;                                     // ��ʕ�
        uint32_t height = 0;                                    // ��ʍ���
        DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM;       // �J���[�t�H�[�}�b�g

        void CreateBackBuffers();
    };

} // namespace Athena