#include "Athena/Resources/Buffer.h"
#include "Athena/Resources/UploadContext.h"
#include "Athena/Utils/Logger.h"
#include <stdexcept>
#include <vector>

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

        // UPLOAD �q�[�v�̏ꍇ�͒��ڏ�������
        if (heapType == D3D12_HEAP_TYPE_UPLOAD) {
            void* mapped = Map();
            memcpy(static_cast<uint8_t*>(mapped) + offset, data, dataSize);
            Unmap();
        }
        else {
            // DEFAULTヒープの場合はUploadWithDeviceメソッドを使用する必要がある
            throw std::runtime_error("Upload to DEFAULT heap requires device and command queue. Use UploadWithDevice() method instead.");
        }
    }

    void Buffer::UploadWithDevice(const void* data, uint64_t dataSize, 
                                ID3D12Device* device, ID3D12CommandQueue* commandQueue,
                                uint64_t offset) {
        if (!data) {
            throw std::invalid_argument("Upload data is null");
        }

        if (!device || !commandQueue) {
            throw std::invalid_argument("Device or command queue is null");
        }

        if (offset + dataSize > size) {
            throw std::out_of_range("Upload size exceeds buffer size");
        }

        // UPLOADヒープの場合は通常のUploadメソッドを使用
        if (heapType == D3D12_HEAP_TYPE_UPLOAD) {
            void* mapped = Map();
            memcpy(static_cast<uint8_t*>(mapped) + offset, data, dataSize);
            Unmap();
            return;
        }

        // DEFAULTヒープの場合はUploadContextを使用
        if (heapType == D3D12_HEAP_TYPE_DEFAULT) {
            UploadContext uploadContext;
            uploadContext.Initialize(device, commandQueue);

            uploadContext.Begin();

            // リソースバリア: COMMON -> COPY_DEST
            uploadContext.TransitionResource(
                resource.Get(),
                D3D12_RESOURCE_STATE_COMMON,
                D3D12_RESOURCE_STATE_COPY_DEST
            );

            // データをアップロード
            if (offset == 0 && dataSize == size) {
                // 全体のアップロード
                uploadContext.UploadBuffer(resource.Get(), data, dataSize);
            } else {
                // 部分的なアップロード（簡易実装）
                // 一時的な全体バッファを作成してコピー
                std::vector<uint8_t> tempBuffer(size);
                
                // 既存データの読み取りは困難なので、ここでは警告を出力
                Logger::Warning("Partial upload to DEFAULT heap may overwrite existing data");
                
                memcpy(tempBuffer.data() + offset, data, dataSize);
                uploadContext.UploadBuffer(resource.Get(), tempBuffer.data(), size);
            }

            // リソースバリア: COPY_DEST -> COMMON
            uploadContext.TransitionResource(
                resource.Get(),
                D3D12_RESOURCE_STATE_COPY_DEST,
                D3D12_RESOURCE_STATE_COMMON
            );

            uploadContext.End();
            uploadContext.Shutdown();

            Logger::Info("Buffer data uploaded to DEFAULT heap (size: %llu bytes, offset: %llu)", dataSize, offset);
        } else {
            throw std::runtime_error("Unsupported heap type for upload");
        }
    }

    void* Buffer::Map() {
        if (mappedData) {
            return mappedData; // ���Ƀ}�b�v�ς�
        }

        D3D12_RANGE readRange = { 0, 0 }; // CPU����ǂ܂Ȃ�
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
        vbv.StrideInBytes = 0; // �Ăяo�����Őݒ肷��K�v����
        return vbv;
    }

    D3D12_INDEX_BUFFER_VIEW Buffer::GetIndexBufferView() const {
        D3D12_INDEX_BUFFER_VIEW ibv = {};
        ibv.BufferLocation = GetGPUVirtualAddress();
        ibv.SizeInBytes = static_cast<UINT>(size);
        ibv.Format = DXGI_FORMAT_R32_UINT; // 32bit �C���f�b�N�X
        return ibv;
    }

    D3D12_CONSTANT_BUFFER_VIEW_DESC Buffer::GetConstantBufferViewDesc() const {
        D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
        cbvDesc.BufferLocation = GetGPUVirtualAddress();
        cbvDesc.SizeInBytes = static_cast<UINT>(size);
        return cbvDesc;
    }

    void Buffer::CreateResource(ID3D12Device* device) {
        // �o�b�t�@���\�[�X�̐ݒ�
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

        // �q�[�v�v���p�e�B
        D3D12_HEAP_PROPERTIES heapProps = {};
        heapProps.Type = heapType;
        heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        heapProps.CreationNodeMask = 1;
        heapProps.VisibleNodeMask = 1;

        // ���\�[�X���
        D3D12_RESOURCE_STATES initialState = D3D12_RESOURCE_STATE_GENERIC_READ;

        // ���\�[�X�쐬
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