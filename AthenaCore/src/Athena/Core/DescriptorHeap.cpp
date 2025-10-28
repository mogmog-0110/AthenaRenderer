#include "Athena/Core/DescriptorHeap.h"
#include "Athena/Utils/Logger.h"
#include <stdexcept>

namespace Athena {

    DescriptorHeap::DescriptorHeap() = default;

    DescriptorHeap::~DescriptorHeap() = default;

    void DescriptorHeap::Initialize(
        ID3D12Device* device,
        D3D12_DESCRIPTOR_HEAP_TYPE type,
        uint32_t capacity,
        bool shaderVisible) {

        this->type = type;
        this->capacity = capacity;
        this->shaderVisible = shaderVisible;

        Logger::Info("Initializing DescriptorHeap (type: %d, capacity: %u, shaderVisible: %s)...",
            type, capacity, shaderVisible ? "Yes" : "No");

        // デスクリプタヒープの設定
        D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
        heapDesc.Type = type;
        heapDesc.NumDescriptors = capacity;
        heapDesc.Flags = shaderVisible ?
            D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE :
            D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        heapDesc.NodeMask = 0;

        // デスクリプタヒープ作成
        HRESULT hr = device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&heap));
        if (FAILED(hr)) {
            throw std::runtime_error("Failed to create descriptor heap");
        }

        // デスクリプタサイズ取得（GPUによって異なる）
        descriptorSize = device->GetDescriptorHandleIncrementSize(type);

        // 開始ハンドル取得
        cpuStart = heap->GetCPUDescriptorHandleForHeapStart();
        if (shaderVisible) {
            gpuStart = heap->GetGPUDescriptorHandleForHeapStart();
        }

        // 割り当て管理の初期化
        allocated.resize(capacity, false);

        Logger::Info("DescriptorHeap initialized (descriptor size: %u bytes)", descriptorSize);
    }

    void DescriptorHeap::Shutdown() {
        Logger::Info("Shutting down DescriptorHeap...");

        allocated.clear();
        heap.Reset();

        Logger::Info("DescriptorHeap shut down successfully");
    }

    DescriptorHandle DescriptorHeap::Allocate() {
        // フリーなインデックスを検索
        for (uint32_t i = nextFreeIndex; i < capacity; ++i) {
            if (!allocated[i]) {
                allocated[i] = true;
                nextFreeIndex = i + 1;

                // ハンドルを計算
                DescriptorHandle handle;
                handle.index = i;
                handle.cpu.ptr = cpuStart.ptr + i * descriptorSize;

                if (shaderVisible) {
                    handle.gpu.ptr = gpuStart.ptr + i * descriptorSize;
                }

                return handle;
            }
        }

        // 最初から再検索
        for (uint32_t i = 0; i < nextFreeIndex; ++i) {
            if (!allocated[i]) {
                allocated[i] = true;
                nextFreeIndex = i + 1;

                DescriptorHandle handle;
                handle.index = i;
                handle.cpu.ptr = cpuStart.ptr + i * descriptorSize;

                if (shaderVisible) {
                    handle.gpu.ptr = gpuStart.ptr + i * descriptorSize;
                }

                return handle;
            }
        }

        throw std::runtime_error("Descriptor heap is full");
    }

    void DescriptorHeap::Free(const DescriptorHandle& handle) {
        if (handle.index < capacity && allocated[handle.index]) {
            allocated[handle.index] = false;

            // 次のフリーインデックスを更新
            if (handle.index < nextFreeIndex) {
                nextFreeIndex = handle.index;
            }
        }
    }

} // namespace Athena