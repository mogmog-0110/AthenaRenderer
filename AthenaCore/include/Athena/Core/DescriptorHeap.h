#pragma once

#include <d3d12.h>
#include <wrl/client.h>
#include <cstdint>
#include <vector>

namespace Athena {

    using Microsoft::WRL::ComPtr;

    /**
     * @brief デスクリプタハンドル
     *
     * CPUハンドルとGPUハンドルの両方を保持。
     * デスクリプタはリソース（テクスチャ、バッファ等）への参照。
     */
    struct DescriptorHandle {
        D3D12_CPU_DESCRIPTOR_HANDLE cpu;  // CPU側ハンドル
        D3D12_GPU_DESCRIPTOR_HANDLE gpu;  // GPU側ハンドル（シェーダー可視の場合のみ）
        uint32_t index = 0;                // ヒープ内のインデックス

        bool IsValid() const { return cpu.ptr != 0; }
        bool IsShaderVisible() const { return gpu.ptr != 0; }
    };

    /**
     * @brief デスクリプタヒープ管理クラス
     *
     * デスクリプタ（リソース記述子）を管理。
     * DirectX 12では、リソースへのアクセスにデスクリプタが必要。
     *
     * デスクリプタヒープの種類：
     * - RTV (Render Target View): レンダーターゲット用
     * - DSV (Depth Stencil View): 深度バッファ用
     * - CBV/SRV/UAV: 定数バッファ、テクスチャ、UAV用
     * - Sampler: サンプラー用
     */
    class DescriptorHeap {
    public:
        DescriptorHeap();
        ~DescriptorHeap();

        /**
         * @brief デスクリプタヒープを初期化
         *
         * @param device D3D12デバイス
         * @param type ヒープのタイプ（RTV, DSV, CBV_SRV_UAV, Sampler）
         * @param capacity ヒープの容量（デスクリプタ数）
         * @param shaderVisible シェーダーから参照可能にするか
         *
         * shader Visibleをtrueにできるのは、CBV_SRV_UAVとSamplerのみ。
         * RTVとDSVは常にシェーダー不可視です。
         */
        void Initialize(
            ID3D12Device* device,
            D3D12_DESCRIPTOR_HEAP_TYPE type,
            uint32_t capacity,
            bool shaderVisible = false
        );

        /**
         * @brief デスクリプタヒープをシャットダウン
         */
        void Shutdown();

        /**
         * @brief デスクリプタを割り当て
         *
         * @return 割り当てられたデスクリプタハンドル
         *
         * ヒープが満杯の場合は例外をスロー。
         */
        DescriptorHandle Allocate();

        /**
         * @brief デスクリプタを解放
         *
         * @param handle 解放するハンドル
         *
         * 解放されたデスクリプタは再利用可能になる。
         */
        void Free(const DescriptorHandle& handle);

        /**
         * @brief D3D12デスクリプタヒープへのアクセス
         */
        ID3D12DescriptorHeap* GetD3D12DescriptorHeap() const { return heap.Get(); }

        /**
         * @brief デスクリプタのサイズを取得
         *
         * @return デスクリプタ1つ分のバイト数
         *
         * このサイズはGPUによって異なる。
         */
        uint32_t GetDescriptorSize() const { return descriptorSize; }

    private:
        ComPtr<ID3D12DescriptorHeap> heap;      // ヒープ本体
        D3D12_DESCRIPTOR_HEAP_TYPE type;        // ヒープのタイプ
        uint32_t capacity = 0;                  // 容量
        uint32_t descriptorSize = 0;            // デスクリプタのサイズ
        bool shaderVisible = false;             // シェーダー可視か

        D3D12_CPU_DESCRIPTOR_HANDLE cpuStart = {};  // CPU側の開始ハンドル
        D3D12_GPU_DESCRIPTOR_HANDLE gpuStart = {};  // GPU側の開始ハンドル

        std::vector<bool> allocated;            // 割り当て状況
        uint32_t nextFreeIndex = 0;             // 次のフリーなインデックス
    };

} // namespace Athena