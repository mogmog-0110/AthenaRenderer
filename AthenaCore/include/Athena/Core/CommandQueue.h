#pragma once

#include <d3d12.h>
#include <wrl/client.h>
#include <cstdint>

namespace Athena {

    using Microsoft::WRL::ComPtr;

    /**
     * @brief コマンドキュー管理クラス
     *
     * GPUへのコマンド送信と、CPU-GPU間の同期を管理。
     * DirectX 12では、コマンドリストをコマンドキューに送信することで
     * GPUに作業を指示。
     */
    class CommandQueue {
    public:
        CommandQueue();
        ~CommandQueue();

        /**
         * @brief コマンドキューを初期化
         *
         * @param device D3D12デバイス
         * @param type コマンドリストのタイプ（Direct/Compute/Copy）
         *
         * DirectX 12では3種類のコマンドキューが存在する：
         * - Direct: グラフィックス＋コンピュート＋コピー（最も一般的）
         * - Compute: コンピュートシェーダー専用
         * - Copy: データ転送専用
         */
        void Initialize(ID3D12Device* device, D3D12_COMMAND_LIST_TYPE type);

        /**
         * @brief コマンドキューをシャットダウン
         *
         * GPUの完了を待ってからリソースを解放。
         */
        void Shutdown();

        /**
         * @brief コマンドリストをGPUに送信して実行
         *
         * @param commandLists 実行するコマンドリストの配列
         * @param count コマンドリストの数
         *
         * コマンドリストは必ず Close() された状態で渡す必要がある。
         */
        void ExecuteCommandLists(ID3D12CommandList* const* commandLists, uint32_t count);

        /**
         * @brief GPUの作業完了を待機
         *
         * CPUがGPUの完了を待つ必要がある場合に使用。
         * 例：フレーム終了時、リソース解放前など
         */
        void WaitForGPU();

        /**
         * @brief フェンスにシグナルを送る
         *
         * GPUが現在のコマンドを完了したときに通知されるよう設定。
         */
        void Signal();

        /**
         * @brief GPUが完了したフェンス値を取得
         *
         * @return 完了したフェンス値
         */
        uint64_t GetCompletedValue() const;

        /**
         * @brief D3D12コマンドキューへのアクセス
         *
         * @return ID3D12CommandQueue ポインタ
         */
        ID3D12CommandQueue* GetD3D12CommandQueue() const { return queue.Get(); }

    private:
        ComPtr<ID3D12CommandQueue> queue;      // コマンドキュー本体
        ComPtr<ID3D12Fence> fence;             // CPU-GPU同期用フェンス
        HANDLE fenceEvent = nullptr;           // フェンス完了通知用イベント
        uint64_t fenceValue = 0;               // 現在のフェンス値
    };

} // namespace Athena