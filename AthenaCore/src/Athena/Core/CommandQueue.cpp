#include "Athena/Core/CommandQueue.h"
#include "Athena/Utils/Logger.h"
#include <stdexcept>

namespace Athena {

    CommandQueue::CommandQueue() = default;

    CommandQueue::~CommandQueue() {
        if (fenceEvent) {
            CloseHandle(fenceEvent);
        }
    }

    void CommandQueue::Initialize(ID3D12Device* device, D3D12_COMMAND_LIST_TYPE type) {
        Logger::Info("Initializing CommandQueue...");

        // �R�}���h�L���[�쐬
        D3D12_COMMAND_QUEUE_DESC queueDesc = {};
        queueDesc.Type = type;                              // �L���[�̃^�C�v
        queueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;  // �D��x
        queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;    // �t���O
        queueDesc.NodeMask = 0;                             // �}���`GPU�p�i0=�f�t�H���g�j

        HRESULT hr = device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&queue));
        if (FAILED(hr)) {
            throw std::runtime_error("Failed to create command queue");
        }

        // �t�F���X�쐬�iCPU-GPU�����p�j
        hr = device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
        if (FAILED(hr)) {
            throw std::runtime_error("Failed to create fence");
        }

        // �C�x���g�I�u�W�F�N�g�쐬�i�t�F���X�����ʒm�p�j
        fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        if (!fenceEvent) {
            throw std::runtime_error("Failed to create fence event");
        }

        Logger::Info("CommandQueue initialized successfully");
    }

    void CommandQueue::Shutdown() {
        Logger::Info("Shutting down CommandQueue...");

        // GPU�̍�Ɗ�����҂�
        WaitForGPU();

        // ���\�[�X���
        if (fenceEvent) {
            CloseHandle(fenceEvent);
            fenceEvent = nullptr;
        }

        fence.Reset();
        queue.Reset();

        Logger::Info("CommandQueue shut down successfully");
    }

    void CommandQueue::ExecuteCommandLists(ID3D12CommandList* const* commandLists, uint32_t count) {
        // �R�}���h���X�g��GPU�ɑ��M
        queue->ExecuteCommandLists(count, commandLists);
    }

    void CommandQueue::Signal() {
        // �t�F���X�l���C���N�������g
        ++fenceValue;

        // GPU�ɃV�O�i���𑗐M
        // ���̒l�ɒB�����Ƃ��Ƀt�F���X���X�V�����
        queue->Signal(fence.Get(), fenceValue);
    }

    void CommandQueue::WaitForGPU() {
        // �V�O�i���𑗐M
        Signal();

        // GPU���܂����̃t�F���X�l�ɒB���Ă��Ȃ��ꍇ
        if (fence->GetCompletedValue() < fenceValue) {
            // �t�F���X�l�ɒB�����Ƃ��ɃC�x���g�𔭉�
            fence->SetEventOnCompletion(fenceValue, fenceEvent);

            // �C�x���g��ҋ@�iGPU�̊�����҂j
            WaitForSingleObject(fenceEvent, INFINITE);
        }
    }

    uint64_t CommandQueue::GetCompletedValue() const {
        return fence->GetCompletedValue();
    }

} // namespace Athena