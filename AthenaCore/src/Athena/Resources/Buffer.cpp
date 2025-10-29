#include "Athena/Resources/Buffer.h"
#include "Athena/Utils/Logger.h"
#include <stdexcept>

namespace Athena {

    Buffer::Buffer() = default;

    Buffer::~Buffer() {
        Shutdown();
    }

    void Buffer::Initialize(
        ID3D12Device* device,
        uint64_t size,
        BufferType type,
        D3D12_HEAP_TYPE heapType) {

        this->size = size;
        this->type = type;
        this->heapType = heapType;

        CreateResource(device);

        Logger::Info("Buffer initialized (size: %llu bytes, type: %d)", size, static_cast<int>(type));
    }

    void Buffer::Shutdown() {
        if (mappedData) {
            Unmap();
        }
        resource.Reset();
    }

    void Buffer::Upload(const void* data, uint64_t dataSize, uint64_t offset) {
        if (!data) {
            throw std::invalid_argument("Upload data is null");
        }

        if (offset + dataSize > size) {
            throw std::out_of_range("Upload size exceeds buffer size");
        }

        // UPLOAD ヒープの場合は直接書き込み
        if (heapType == D3D12_HEAP_TYPE_UPLOAD) {
            void* mapped = Map();
            memcpy(static_cast<uint8_t*>(mapped) + offset, data, dataSize);
            Unmap();
        }
        else {
            // DEFAULT ヒープの場合は一時的なアップロードバッファ経由
            // TODO: UploadContext を使った実装（後のフェーズで）
            throw std::runtime_error("Upload to DEFAULT heap not yet implemented");
        }
    }

    void* Buffer::Map() {
        if (mappedData) {
            return mappedData; // 既にマップ済み
        }

        D3D12_RANGE readRange = { 0, 0 }; // CPUから読まない
        HRESULT hr = resource->Map(0, &readRange, &mappedData);

        if (FAILED(hr)) {
            throw std::runtime_error("Failed to map buffer");
        }

        return mappedData;
    }

    void Buffer::Unmap() {
        if (mappedData) {
            resource->Unmap(0, nullptr);
            mappedData = nullptr;
        }
    }

    D3D12_GPU_VIRTUAL_ADDRESS Buffer::GetGPUVirtualAddress() const {
        return resource ? resource->GetGPUVirtualAddress() : 0;
    }

    D3D12_VERTEX_BUFFER_VIEW Buffer::GetVertexBufferView() const {
        D3D12_VERTEX_BUFFER_VIEW vbv = {};
        vbv.BufferLocation = GetGPUVirtualAddress();
        vbv.SizeInBytes = static_cast<UINT>(size);
        vbv.StrideInBytes = 0; // 呼び出し側で設定する必要あり
        return vbv;
    }

    D3D12_INDEX_BUFFER_VIEW Buffer::GetIndexBufferView() const {
        D3D12_INDEX_BUFFER_VIEW ibv = {};
        ibv.BufferLocation = GetGPUVirtualAddress();
        ibv.SizeInBytes = static_cast<UINT>(size);
        ibv.Format = DXGI_FORMAT_R32_UINT; // 32bit インデックス
        return ibv;
    }

    D3D12_CONSTANT_BUFFER_VIEW_DESC Buffer::GetConstantBufferViewDesc() const {
        D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
        cbvDesc.BufferLocation = GetGPUVirtualAddress();
        cbvDesc.SizeInBytes = static_cast<UINT>(size);
        return cbvDesc;
    }

    void Buffer::CreateResource(ID3D12Device* device) {
        // バッファリソースの設定
        D3D12_RESOURCE_DESC resourceDesc = {};
        resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        resourceDesc.Alignment = 0;
        resourceDesc.Width = size;
        resourceDesc.Height = 1;
        resourceDesc.DepthOrArraySize = 1;
        resourceDesc.MipLevels = 1;
        resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
        resourceDesc.SampleDesc.Count = 1;
        resourceDesc.SampleDesc.Quality = 0;
        resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

        // ヒーププロパティ
        D3D12_HEAP_PROPERTIES heapProps = {};
        heapProps.Type = heapType;
        heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        heapProps.CreationNodeMask = 1;
        heapProps.VisibleNodeMask = 1;

        // リソース状態
        D3D12_RESOURCE_STATES initialState = D3D12_RESOURCE_STATE_GENERIC_READ;

        // リソース作成
        HRESULT hr = device->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &resourceDesc,
            initialState,
            nullptr,
            IID_PPV_ARGS(&resource)
        );

        if (FAILED(hr)) {
            throw std::runtime_error("Failed to create buffer resource");
        }
    }

} // namespace Athena