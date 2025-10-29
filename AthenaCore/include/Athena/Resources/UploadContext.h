#pragma once

#include <d3d12.h>
#include <wrl/client.h>
#include <cstdint>
#include <vector>
#include <functional>

namespace Athena {

    using Microsoft::WRL::ComPtr;

    /**
     * @brief アップロードコンテキスト
     *
     * テクスチャやバッファをGPUにアップロードするための
     * 一時的なコマンドリストとリソース管理を行う
     */
    class UploadContext {
    public:
        UploadContext();
        ~UploadContext();

        /**
         * @brief 初期化
         * @param device DirectX 12デバイス
         * @param commandQueue コマンドキュー
         */
        void Initialize(ID3D12Device* device, ID3D12CommandQueue* commandQueue);

        /**
         * @brief 終了処理
         */
        void Shutdown();

        /**
         * @brief アップロードを開始
         * アップロード用コマンドリストを開始状態にする
         */
        void Begin();

        /**
         * @brief バッファデータをアップロード
         * @param destinationResource 転送先リソース
         * @param data アップロードするデータ
         * @param dataSize データサイズ
         */
        void UploadBuffer(
            ID3D12Resource* destinationResource,
            const void* data,
            uint64_t dataSize
        );

        /**
         * @brief テクスチャデータをアップロード
         * @param destinationResource 転送先テクスチャリソース
         * @param subresourceData サブリソースデータ
         * @param numSubresources サブリソース数
         */
        void UploadTexture(
            ID3D12Resource* destinationResource,
            const D3D12_SUBRESOURCE_DATA* subresourceData,
            uint32_t numSubresources
        );

        /**
         * @brief リソースバリアを追加
         * @param resource 対象リソース
         * @param stateBefore 変更前の状態
         * @param stateAfter 変更後の状態
         */
        void TransitionResource(
            ID3D12Resource* resource,
            D3D12_RESOURCE_STATES stateBefore,
            D3D12_RESOURCE_STATES stateAfter
        );

        /**
         * @brief アップロードを終了してGPUに送信
         * コマンドリストを閉じて実行し、完了を待機する
         */
        void End();

        /**
         * @brief コマンドリストへのアクセス
         */
        ID3D12GraphicsCommandList* GetCommandList() const { return commandList.Get(); }

    private:
        ComPtr<ID3D12Device> device;
        ComPtr<ID3D12CommandQueue> commandQueue;
        ComPtr<ID3D12CommandAllocator> commandAllocator;
        ComPtr<ID3D12GraphicsCommandList> commandList;

        ComPtr<ID3D12Fence> fence;
        HANDLE fenceEvent = nullptr;
        uint64_t fenceValue = 0;

        // 一時的なアップロードバッファ（End()で自動削除）
        std::vector<ComPtr<ID3D12Resource>> uploadBuffers;

        void WaitForGPU();
        void ResetCommandList();
    };

} // namespace Athena