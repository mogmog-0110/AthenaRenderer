#pragma once

#include <d3d12.h>
#include <wrl/client.h>
#include <cstdint>

namespace Athena {

    using Microsoft::WRL::ComPtr;

    /**
     * @brief バッファの種類
     */
    enum class BufferType {
        Vertex,      // 頂点バッファ
        Index,       // インデックスバッファ
        Constant,    // 定数バッファ
        Structured   // 構造化バッファ
    };

    /**
     * @brief GPU バッファの抽象化クラス
     *
     * DirectX 12の様々なバッファ（頂点、インデックス、定数など）を
     * 統一的に扱うための基底クラス
     */
    class Buffer {
    public:
        Buffer();
        ~Buffer();

        /**
         * @brief バッファの初期化
         * @param device DirectX 12デバイス
         * @param size バッファサイズ（バイト）
         * @param type バッファの種類
         * @param heapType ヒープタイプ（DEFAULT or UPLOAD）
         */
        void Initialize(
            ID3D12Device* device,
            uint64_t size,
            BufferType type,
            D3D12_HEAP_TYPE heapType = D3D12_HEAP_TYPE_DEFAULT
        );

        /**
         * @brief バッファの終了処理
         */
        void Shutdown();

        /**
         * @brief データをバッファにアップロード
         * @param data アップロードするデータ
         * @param size データサイズ（バイト）
         * @param offset バッファ内のオフセット
         */
        void Upload(const void* data, uint64_t size, uint64_t offset = 0);

        /**
         * @brief バッファをマップ（CPUアクセス可能にする）
         * @return マップされたメモリへのポインタ
         */
        void* Map();

        /**
         * @brief バッファをアンマップ
         */
        void Unmap();

        // アクセサ
        ID3D12Resource* GetResource() const { return resource.Get(); }
        D3D12_GPU_VIRTUAL_ADDRESS GetGPUVirtualAddress() const;
        uint64_t GetSize() const { return size; }
        BufferType GetType() const { return type; }

        // ビュー取得
        D3D12_VERTEX_BUFFER_VIEW GetVertexBufferView() const;
        D3D12_INDEX_BUFFER_VIEW GetIndexBufferView() const;
        D3D12_CONSTANT_BUFFER_VIEW_DESC GetConstantBufferViewDesc() const;

    protected:
        ComPtr<ID3D12Resource> resource;
        uint64_t size = 0;
        BufferType type;
        D3D12_HEAP_TYPE heapType;
        void* mappedData = nullptr;

    private:
        void CreateResource(ID3D12Device* device);
    };

} // namespace Athena