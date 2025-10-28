#include "Athena/Core/SwapChain.h"
#include "Athena/Utils/Logger.h"
#include <stdexcept>

namespace Athena {

    SwapChain::SwapChain() = default;

    SwapChain::~SwapChain() = default;

    void SwapChain::Initialize(
        IDXGIFactory6* factory,
        ID3D12CommandQueue* commandQueue,
        HWND hwnd,
        uint32_t width,
        uint32_t height) {

        this->width = width;
        this->height = height;

        Logger::Info("Initializing SwapChain (%ux%u)...", width, height);

        // �X���b�v�`�F�[���̐ݒ�
        DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
        swapChainDesc.Width = width;                               // ��
        swapChainDesc.Height = height;                             // ����
        swapChainDesc.Format = format;                             // �s�N�Z���t�H�[�}�b�g
        swapChainDesc.Stereo = FALSE;                              // �X�e���I�\���iVR�p�j
        swapChainDesc.SampleDesc.Count = 1;                        // MSAA�T���v�����i1=�����j
        swapChainDesc.SampleDesc.Quality = 0;                      // MSAA�i��
        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;  // �����_�[�^�[�Q�b�g�p
        swapChainDesc.BufferCount = BufferCount;                   // �o�b�N�o�b�t�@���i3���j
        swapChainDesc.Scaling = DXGI_SCALING_STRETCH;              // �X�P�[�����O���@
        swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;  // �t���b�v���f��
        swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;     // �A���t�@����
        swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;  // �e�B�A�����O����

        // �X���b�v�`�F�[���쐬�i�܂�IDXGISwapChain1�Ƃ��č쐬�j
        ComPtr<IDXGISwapChain1> swapChain1;
        HRESULT hr = factory->CreateSwapChainForHwnd(
            commandQueue,           // �R�}���h�L���[
            hwnd,                   // �E�B���h�E�n���h��
            &swapChainDesc,         // �X���b�v�`�F�[���ݒ�
            nullptr,                // �t���X�N���[���ݒ�inullptr=�E�B���h�E���[�h�j
            nullptr,                // �o�͐����inullptr=�����Ȃ��j
            &swapChain1
        );

        if (FAILED(hr)) {
            throw std::runtime_error("Failed to create swap chain");
        }

        // IDXGISwapChain4�ɃL���X�g�i�ŐV�C���^�[�t�F�[�X�j
        swapChain1.As(&swapChain);

        // Alt+Enter�ł̃t���X�N���[���؂�ւ��𖳌���
        // �i�蓮�Ńt���X�N���[���Ǘ��������ꍇ�j
        factory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER);

        // �o�b�N�o�b�t�@�̎擾
        CreateBackBuffers();

        Logger::Info("SwapChain initialized successfully");
    }

    void SwapChain::Shutdown() {
        Logger::Info("Shutting down SwapChain...");

        backBuffers.clear();
        swapChain.Reset();

        Logger::Info("SwapChain shut down successfully");
    }

    void SwapChain::Present(bool vsync) {
        // ���������̐ݒ�
        UINT syncInterval = vsync ? 1 : 0;
        UINT flags = vsync ? 0 : DXGI_PRESENT_ALLOW_TEARING;

        // ��ʂɕ\��
        swapChain->Present(syncInterval, flags);
    }

    uint32_t SwapChain::GetCurrentBackBufferIndex() const {
        return swapChain->GetCurrentBackBufferIndex();
    }

    ID3D12Resource* SwapChain::GetCurrentBackBuffer() const {
        return backBuffers[GetCurrentBackBufferIndex()].Get();
    }

    ID3D12Resource* SwapChain::GetBackBuffer(uint32_t index) const {
        if (index >= BufferCount) {
            Logger::Error("Invalid back buffer index: %u", index);
            return nullptr;
        }
        return backBuffers[index].Get();
    }

    void SwapChain::Resize(uint32_t width, uint32_t height) {
        // �T�C�Y���ς���Ă��Ȃ���Ή������Ȃ�
        if (this->width == width && this->height == height) {
            return;
        }

        Logger::Info("Resizing SwapChain to %ux%u...", width, height);

        this->width = width;
        this->height = height;

        // �o�b�N�o�b�t�@����x���
        backBuffers.clear();

        // �X���b�v�`�F�[���̃��T�C�Y
        HRESULT hr = swapChain->ResizeBuffers(
            BufferCount,                            // �o�b�t�@��
            width,                                  // �V������
            height,                                 // �V��������
            format,                                 // �t�H�[�}�b�g
            DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING     // �t���O
        );

        if (FAILED(hr)) {
            throw std::runtime_error("Failed to resize swap chain");
        }

        // �o�b�N�o�b�t�@���Ď擾
        CreateBackBuffers();

        Logger::Info("SwapChain resized successfully");
    }

    void SwapChain::CreateBackBuffers() {
        backBuffers.resize(BufferCount);

        // �e�o�b�N�o�b�t�@���擾
        for (uint32_t i = 0; i < BufferCount; ++i) {
            HRESULT hr = swapChain->GetBuffer(i, IID_PPV_ARGS(&backBuffers[i]));
            if (FAILED(hr)) {
                throw std::runtime_error("Failed to get back buffer");
            }

            // �f�o�b�O�p�̖��O��ݒ�iPIX�Ȃǂŕ\�������j
            wchar_t name[32];
            swprintf_s(name, L"BackBuffer[%u]", i);
            backBuffers[i]->SetName(name);
        }

        Logger::Info("Created %u back buffers", BufferCount);
    }

} // namespace Athena