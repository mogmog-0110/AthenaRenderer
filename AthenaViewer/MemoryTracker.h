#pragma once

#include <Windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
#include <psapi.h>
#include <memory>
#include <atomic>

namespace Athena {

    using Microsoft::WRL::ComPtr;

    /**
     * DirectX 12リソースのメモリ使用量を追跡するクラス
     */
    class MemoryTracker {
    public:
        MemoryTracker();
        ~MemoryTracker();

        /**
         * バッファリソースのメモリ使用量を記録
         */
        void AddBufferMemory(uint64_t sizeInBytes);

        /**
         * テクスチャリソースのメモリ使用量を記録
         */
        void AddTextureMemory(uint64_t sizeInBytes);

        /**
         * 記録されたメモリ使用量を削除
         */
        void RemoveMemory(uint64_t sizeInBytes);

        /**
         * DirectX 12リソースの合計メモリ使用量を取得（MB単位）
         */
        float GetD3D12MemoryUsageMB() const;

        /**
         * プロセス全体のメモリ使用量を取得（MB単位）
         */
        float GetProcessMemoryUsageMB() const;

        /**
         * GPU専用メモリの使用量を取得（MB単位）
         */
        float GetGPUMemoryUsageMB() const;

        /**
         * 統計情報をリセット
         */
        void Reset();

    private:
        std::atomic<uint64_t> totalBufferMemory;
        std::atomic<uint64_t> totalTextureMemory;
        ComPtr<IDXGIAdapter3> dxgiAdapter;
        
        /**
         * DXGI アダプターを初期化
         */
        bool InitializeDXGIAdapter();
    };

} // namespace Athena