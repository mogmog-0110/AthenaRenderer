#include "Athena/Core/Device.h"
#include "Athena/Utils/Logger.h"
#include <stdexcept>

namespace Athena {

    Device::Device() = default;  // �f�t�H���g�R���X�g���N�^

    Device::~Device() = default;  // �f�t�H���g�f�X�g���N�^

    void Device::Initialize(bool enableDebugLayer) {
        Logger::Info("Initializing Device...");

        // �f�o�b�O���C���[�̗L�����i�J�����̂ݐ����j
        if (enableDebugLayer) {
            EnableDebugLayer();
        }

        // DXGI�t�@�N�g���쐬
        // DXGI = DirectX Graphics Infrastructure
        // GPU�̗񋓂�SwapChain�̍쐬�Ȃǂ�S��
        UINT flags = 0;
        if (enableDebugLayer) {
            flags = DXGI_CREATE_FACTORY_DEBUG;
        }

        HRESULT hr = CreateDXGIFactory2(flags, IID_PPV_ARGS(&factory));
        if (FAILED(hr)) {
            throw std::runtime_error("Failed to create DXGI factory");
        }

        // �œK�ȃA�_�v�^�[�iGPU�j��I��
        SelectBestAdapter();

        // D3D12�f�o�C�X�̍쐬
        // D3D_FEATURE_LEVEL_12_0 = DirectX 12�Ή���v��
        hr = D3D12CreateDevice(
            adapter.Get(),              // �g�p����GPU
            D3D_FEATURE_LEVEL_12_0,    // �v������@�\���x��
            IID_PPV_ARGS(&device)      // �쐬���ꂽ�f�o�C�X���i�[
        );

        if (FAILED(hr)) {
            throw std::runtime_error("Failed to create D3D12 device");
        }

        // �@�\�T�|�[�g�̃`�F�b�N
        CheckFeatureSupport();

        Logger::Info("Device initialized successfully");
    }

    void Device::Shutdown() {
        Logger::Info("Shutting down Device...");

        // ComPtr�͎����I��Release���ĂԂ��A
        // �����I�Ƀ��Z�b�g���邱�Ƃŏ����𐧌�
        device.Reset();
        adapter.Reset();
        factory.Reset();
    }

    void Device::EnableDebugLayer() {
#ifdef _DEBUG  // �f�o�b�O�r���h�̎��̂ݗL��
        ComPtr<ID3D12Debug> debugController;

        // �f�o�b�O�C���^�[�t�F�[�X���擾
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
            // �f�o�b�O���C���[��L����
            // ����ɂ��ADirectX 12�̃G���[��x�����ڍׂɕ\��
            debugController->EnableDebugLayer();
            Logger::Info("D3D12 Debug Layer enabled");
        }
#endif
    }

    void Device::SelectBestAdapter() {
        ComPtr<IDXGIAdapter1> tempAdapter;
        SIZE_T maxVideoMemory = 0;  // �ő�VRAM�e��

        // ���ׂẴA�_�v�^�[�iGPU�j���
        for (UINT i = 0;
            factory->EnumAdapters1(i, &tempAdapter) != DXGI_ERROR_NOT_FOUND;
            ++i) {

            DXGI_ADAPTER_DESC1 desc;
            tempAdapter->GetDesc1(&desc);

            // �\�t�g�E�F�A�A�_�v�^�[�i���zGPU�j�̓X�L�b�v
            if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
                continue;
            }

            // DirectX 12�Ή����`�F�b�N
            if (SUCCEEDED(D3D12CreateDevice(
                tempAdapter.Get(),
                D3D_FEATURE_LEVEL_12_0,
                __uuidof(ID3D12Device),
                nullptr))) {  // nullptr = ���ۂɂ͍쐬���Ȃ��i�`�F�b�N�̂݁j

                // �ő�VRAM������GPU��I��
                if (desc.DedicatedVideoMemory > maxVideoMemory) {
                    maxVideoMemory = desc.DedicatedVideoMemory;
                    tempAdapter.As(&adapter);  // ComPtr�̃L���X�g
                }
            }
        }

        if (!adapter) {
            throw std::runtime_error("No compatible adapter found");
        }

        // �I�����ꂽGPU�������O�o��
        DXGI_ADAPTER_DESC1 desc;
        adapter->GetDesc1(&desc);

        Logger::Info("Selected GPU: %S", desc.Description);  // %S = ���C�h������
        Logger::Info("VRAM: %.2f GB",
            desc.DedicatedVideoMemory / (1024.0 * 1024.0 * 1024.0));
    }

    void Device::CheckFeatureSupport() {
        // Mesh Shader�Ή��̃`�F�b�N
        D3D12_FEATURE_DATA_D3D12_OPTIONS7 options7 = {};
        if (SUCCEEDED(device->CheckFeatureSupport(
            D3D12_FEATURE_D3D12_OPTIONS7,
            &options7, sizeof(options7)))) {
            supportMeshShaders = (options7.MeshShaderTier >= D3D12_MESH_SHADER_TIER_1);
        }

        // Raytracing�Ή��̃`�F�b�N
        D3D12_FEATURE_DATA_D3D12_OPTIONS5 options5 = {};
        if (SUCCEEDED(device->CheckFeatureSupport(
            D3D12_FEATURE_D3D12_OPTIONS5,
            &options5, sizeof(options5)))) {
            supportRaytracing = (options5.RaytracingTier >= D3D12_RAYTRACING_TIER_1_0);
        }

        // Variable Rate Shading�Ή��̃`�F�b�N
        D3D12_FEATURE_DATA_D3D12_OPTIONS6 options6 = {};
        if (SUCCEEDED(device->CheckFeatureSupport(
            D3D12_FEATURE_D3D12_OPTIONS6,
            &options6, sizeof(options6)))) {
            supportVRS = (options6.VariableShadingRateTier >=
                D3D12_VARIABLE_SHADING_RATE_TIER_1);
        }

        // �T�|�[�g�󋵂����O�o��
        Logger::Info("Feature Support:");
        Logger::Info("  Mesh Shaders: %s", supportMeshShaders ? "Yes" : "No");
        Logger::Info("  Raytracing: %s", supportRaytracing ? "Yes" : "No");
        Logger::Info("  VRS: %s", supportVRS ? "Yes" : "No");
    }

} // namespace Athena