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

        // コマンドキュー作成
        D3D12_COMMAND_QUEUE_DESC queueDesc = {};
        queueDesc.Type = type;                              // キューのタイプ
        queueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;  // 優先度
        queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;    // フラグ
        queueDesc.NodeMask = 0;                             // マルチGPU用（0=デフォルト）

        HRESULT hr = device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&queue));
        if (FAILED(hr)) {
            throw std::runtime_error("Failed to create command queue");
        }

        // フェンス作成（CPU-GPU同期用）
        hr = device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
        if (FAILED(hr)) {
            throw std::runtime_error("Failed to create fence");
        }

        // イベントオブジェクト作成（フェンス完了通知用）
        fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        if (!fenceEvent) {
            throw std::runtime_error("Failed to create fence event");
        }

        Logger::Info("CommandQueue initialized successfully");
    }

    void CommandQueue::Shutdown() {
        Logger::Info("Shutting down CommandQueue...");

        // GPUの作業完了を待つ
        WaitForGPU();

        // リソース解放
        if (fenceEvent) {
            CloseHandle(fenceEvent);
            fenceEvent = nullptr;
        }

        fence.Reset();
        queue.Reset();

        Logger::Info("CommandQueue shut down successfully");
    }

    void CommandQueue::ExecuteCommandLists(ID3D12CommandList* const* commandLists, uint32_t count) {
        // コマンドリストをGPUに送信
        queue->ExecuteCommandLists(count, commandLists);
    }

    void CommandQueue::Signal() {
        // フェンス値をインクリメント
        ++fenceValue;

        // GPUにシグナルを送信
        // この値に達したときにフェンスが更新される
        queue->Signal(fence.Get(), fenceValue);
    }

    void CommandQueue::WaitForGPU() {
        // シグナルを送信
        Signal();

        // GPUがまだこのフェンス値に達していない場合
        if (fence->GetCompletedValue() < fenceValue) {
            // フェンス値に達したときにイベントを発火
            fence->SetEventOnCompletion(fenceValue, fenceEvent);

            // イベントを待機（GPUの完了を待つ）
            WaitForSingleObject(fenceEvent, INFINITE);
        }
    }

    uint64_t CommandQueue::GetCompletedValue() const {
        return fence->GetCompletedValue();
    }

} // namespace Athena