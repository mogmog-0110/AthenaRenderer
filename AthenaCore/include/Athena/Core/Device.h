#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>

namespace Athena {

    // ComPtr: COM�I�u�W�F�N�g�p�̃X�}�[�g�|�C���^
    // �����I��AddRef/Release���Ă�ł����̂ŁA���������[�N��h����
    using Microsoft::WRL::ComPtr;

    /**
     * @brief DirectX 12�f�o�C�X�̊Ǘ��N���X
     *
     * GPU�Ƃ̒ʐM���s�����߂̒��S�I�ȃN���X�B
     * - GPU�A�_�v�^�[�i����GPU�j�̑I��
     * - ID3D12Device�iDirectX 12�̒��j�I�u�W�F�N�g�j�̍쐬
     * - �@�\�T�|�[�g�̃`�F�b�N�iMesh Shader�ARaytracing�Ȃǁj
     */
    class Device {
    public:
        Device();
        ~Device();

        /**
         * @brief �f�o�C�X��������
         * @param enableDebugLayer �f�o�b�O���C���[��L���ɂ��邩
         *                         �i�J������true�A�����[�X����false�j
         */
        void Initialize(bool enableDebugLayer = true);

        /**
         * @brief �f�o�C�X���V���b�g�_�E��
         */
        void Shutdown();

        // �A�N�Z�T�i�Q�b�^�[�֐��j
        // ������ʂ��đ��̃N���X��DirectX 12�̃I�u�W�F�N�g�ɃA�N�Z�X
        ID3D12Device5* GetD3D12Device() const { return device.Get(); }
        IDXGIFactory6* GetDXGIFactory() const { return factory.Get(); }
        IDXGIAdapter4* GetAdapter() const { return adapter.Get(); }

        // �@�\�T�|�[�g�m�F
        // GPU��Mesh Shader��Raytracing�ɑΉ����Ă��邩���m�F
        bool SupportsMeshShaders() const { return supportMeshShaders; }
        bool SupportsRaytracing() const { return supportRaytracing; }
        bool SupportsVariableRateShading() const { return supportVRS; }

    private:
        // ComPtr<T>: COM�I�u�W�F�N�g�������Ǘ�����X�}�[�g�|�C���^
        ComPtr<ID3D12Device5> device;        // DirectX 12�f�o�C�X�{��
        ComPtr<IDXGIFactory6> factory;       // �f�o�C�X�쐬��SwapChain�쐬�p
        ComPtr<IDXGIAdapter4> adapter;       // ����GPU�i�O���t�B�b�N�J�[�h�j

        // �@�\�T�|�[�g�t���O
        bool supportMeshShaders = false;
        bool supportRaytracing = false;
        bool supportVRS = false;  // VRS = Variable Rate Shading

        // �����w���p�[�֐�
        void EnableDebugLayer();           // �f�o�b�O���C���[�̗L����
        void SelectBestAdapter();          // �œK��GPU��I��
        void CheckFeatureSupport();        // �@�\�T�|�[�g���`�F�b�N
    };

} // namespace Athena