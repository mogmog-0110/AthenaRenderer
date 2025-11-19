#pragma once

#include <d3d12.h>
#include <wrl/client.h>
#include <cstdint>
#include <string>
#include <DirectXTex.h>

namespace Athena {

    using Microsoft::WRL::ComPtr;

    // �O���錾
    class UploadContext;

    /**
     * @brief �e�N�X�`���̎��
     */
    enum class TextureType {
        Texture2D,      // 2D�e�N�X�`��
        TextureCube,    // �L���[�u�}�b�v
        Texture3D,      // 3D�e�N�X�`��
        RenderTarget,   // �����_�[�^�[�Q�b�g
        DepthStencil    // �[�x�X�e���V��
    };

    /**
     * @brief �e�N�X�`�����ۉ��N���X
     *
     * DirectXTex���g�p���ėl�X�Ȍ`���̉摜�t�@�C����ǂݍ��݁A
     * DirectX 12�̃e�N�X�`�����\�[�X�Ƃ��ĊǗ�����
     */
    class Texture {
    public:
        Texture();
        ~Texture();

        /**
         * @brief �摜�t�@�C������ǂݍ��݁iNEW!�j
         * @param device DirectX 12�f�o�C�X
         * @param filepath �摜�t�@�C���p�X�i.png, .jpg, .dds���j
         * @param uploadContext �A�b�v���[�h�R���e�L�X�g
         * @param generateMips �~�b�v�}�b�v�������������邩
         */
        void LoadFromFile(
            ID3D12Device* device,
            const wchar_t* filepath,
            UploadContext* uploadContext,
            bool generateMips = true
        );

        /**
         * @brief GPU�Ƀe�N�X�`�����A�b�v���[�h
         * @param uploadContext �A�b�v���[�h�R���e�L�X�g
         *
         * LoadFromFile()�̌�ɌĂяo���āA���ۂ�GPU�Ƀf�[�^��]������
         */
        void UploadToGPU(UploadContext* uploadContext);

        /**
         * @brief ����������e�N�X�`�����쐬
         * @param device DirectX 12�f�o�C�X
         * @param width �e�N�X�`����
         * @param height �e�N�X�`������
         * @param format �s�N�Z���t�H�[�}�b�g
         * @param data �摜�f�[�^
         */
        void CreateFromMemory(
            ID3D12Device* device,
            uint32_t width,
            uint32_t height,
            DXGI_FORMAT format,
            const void* data
        );

        /**
         * @brief �����_�[�^�[�Q�b�g�p�e�N�X�`�����쐬
         */
        void CreateRenderTarget(
            ID3D12Device* device,
            uint32_t width,
            uint32_t height,
            DXGI_FORMAT format
        );

        /**
         * @brief �[�x�X�e���V���p�e�N�X�`�����쐬
         */
        void CreateDepthStencil(
            ID3D12Device* device,
            uint32_t width,
            uint32_t height
        );

        /**
         * @brief �e�N�X�`���̏I������
         */
        void Shutdown();

        /**
         * @brief Shader Resource View���쐬
         * @param device DirectX 12�f�o�C�X
         * @param cpuHandle SRV���쐬����f�X�N���v�^�n���h��
         */
        void CreateSRV(ID3D12Device* device, D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle);

        /**
         * @brief Render Target View���쐬
         * @param device DirectX 12�f�o�C�X
         * @param cpuHandle RTV���쐬����f�X�N���v�^�n���h��
         */
        void CreateRTV(ID3D12Device* device, D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle);

        /**
         * @brief Depth Stencil View���쐬
         * @param device DirectX 12�f�o�C�X
         * @param cpuHandle DSV���쐬����f�X�N���v�^�n���h��
         */
        void CreateDSV(ID3D12Device* device, D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle);

        // �A�N�Z�T
        ID3D12Resource* GetResource() const { return resource.Get(); }
        ID3D12Resource* GetD3D12Resource() const { return resource.Get(); }  // RenderGraphバリア用
        uint32_t GetWidth() const { return width; }
        uint32_t GetHeight() const { return height; }
        DXGI_FORMAT GetFormat() const { return format; }
        TextureType GetType() const { return type; }
        
        // タイプチェック
        bool IsRenderTarget() const { return type == TextureType::RenderTarget; }
        bool IsDepthStencil() const { return type == TextureType::DepthStencil; }

    private:
        ComPtr<ID3D12Resource> resource;
        ComPtr<ID3D12Resource> uploadBuffer; // �A�b�v���[�h�p�ꎞ�o�b�t�@

        uint32_t width = 0;
        uint32_t height = 0;
        DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
        TextureType type = TextureType::Texture2D;
        uint32_t mipLevels = 1;

        // DirectXTex�̈ꎞ�摜�f�[�^�i�A�b�v���[�h�O�j
        DirectX::ScratchImage tempImageData;

        void CreateResource(
            ID3D12Device* device,
            uint32_t width,
            uint32_t height,
            DXGI_FORMAT format,
            D3D12_RESOURCE_FLAGS flags,
            D3D12_RESOURCE_STATES initialState,
            const D3D12_CLEAR_VALUE* clearValue = nullptr
        );
    };

} // namespace Athena