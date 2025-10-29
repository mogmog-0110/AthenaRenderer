#include "Athena/Resources/UploadContext.h"
#include "Athena/Utils/Logger.h"
#include <stdexcept>

namespace Athena {

    UploadContext::UploadContext() = default;

    UploadContext::~UploadContext() {
        //Shutdown();
    }

    void UploadContext::Initialize(ID3D12Device* device, ID3D12CommandQueue* commandQueue) {
        this->device = device;
        this->commandQueue = commandQueue;

        // コマンドアロケータ作成
        HRESULT hr = device->CreateCommandAllocator(
            D3D12_COMMAND_LIST_TYPE_DIRECT,
            IID_PPV_ARGS(&commandAllocator)
        );
        if (FAILED(hr)) {
            throw std::runtime_error("Failed to create command allocator for upload context");
        }

        // コマンドリスト作成
        hr = device->CreateCommandList(
            0,
            D3D12_COMMAND_LIST_TYPE_DIRECT,
            commandAllocator.Get(),
            nullptr,
            IID_PPV_ARGS(&commandList)
        );
        if (FAILED(hr)) {
            throw std::runtime_error("Failed to create command list for upload context");
        }

        // 初期状態は閉じておく
        commandList->Close();

        // フェンス作成
        hr = device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
        if (FAILED(hr)) {
            throw std::runtime_error("Failed to create fence for upload context");
        }

        // イベント作成
        fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        if (!fenceEvent) {
            throw std::runtime_error("Failed to create fence event for upload context");
        }

        Logger::Info("UploadContext initialized");
    }

    void UploadContext::Shutdown() {
        if (!commandQueue) {
            return;  // 既に解放済み
        }

        // アップロードバッファをクリア
        uploadBuffers.clear();

        // イベントハンドルを解放
        if (fenceEvent) {
            CloseHandle(fenceEvent);
            fenceEvent = nullptr;
        }

        // ComPtrのリソースを解放
        fence.Reset();
        commandList.Reset();
        commandAllocator.Reset();

        commandQueue = nullptr;
        device = nullptr;
    }

    void UploadContext::Begin() {
        // コマンドアロケータをリセット
        commandAllocator->Reset();

        // コマンドリストをリセット
        commandList->Reset(commandAllocator.Get(), nullptr);

        // 前回のアップロードバッファをクリア
        uploadBuffers.clear();
    }

    void UploadContext::UploadBuffer(
        ID3D12Resource* destinationResource,
        const void* data,
        uint64_t dataSize) {

        // アップロードバッファ作成
        D3D12_HEAP_PROPERTIES uploadHeapProps = {};
        uploadHeapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

        D3D12_RESOURCE_DESC uploadBufferDesc = {};
        uploadBufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        uploadBufferDesc.Width = dataSize;
        uploadBufferDesc.Height = 1;
        uploadBufferDesc.DepthOrArraySize = 1;
        uploadBufferDesc.MipLevels = 1;
        uploadBufferDesc.Format = DXGI_FORMAT_UNKNOWN;
        uploadBufferDesc.SampleDesc.Count = 1;
        uploadBufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

        ComPtr<ID3D12Resource> uploadBuffer;
        HRESULT hr = device->CreateCommittedResource(
            &uploadHeapProps,
            D3D12_HEAP_FLAG_NONE,
            &uploadBufferDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&uploadBuffer)
        );

        if (FAILED(hr)) {
            throw std::runtime_error("Failed to create upload buffer");
        }

        // データをアップロードバッファにコピー
        void* mappedData = nullptr;
        uploadBuffer->Map(0, nullptr, &mappedData);
        memcpy(mappedData, data, dataSize);
        uploadBuffer->Unmap(0, nullptr);

        // GPUにコピー
        commandList->CopyBufferRegion(destinationResource, 0, uploadBuffer.Get(), 0, dataSize);

        // アップロードバッファを保持（End()まで）
        uploadBuffers.push_back(uploadBuffer);
    }

    void UploadContext::UploadTexture(
        ID3D12Resource* destinationResource,
        const D3D12_SUBRESOURCE_DATA* subresourceData,
        uint32_t numSubresources) {

        // 必要なアップロードバッファサイズを計算
        uint64_t uploadBufferSize = 0;
        D3D12_RESOURCE_DESC destDesc = destinationResource->GetDesc();

        // 各サブリソースのサイズを計算
        std::vector<D3D12_PLACED_SUBRESOURCE_FOOTPRINT> layouts(numSubresources);
        std::vector<uint32_t> numRows(numSubresources);
        std::vector<uint64_t> rowSizeInBytes(numSubresources);

        device->GetCopyableFootprints(
            &destDesc,
            0,
            numSubresources,
            0,
            layouts.data(),
            numRows.data(),
            rowSizeInBytes.data(),
            &uploadBufferSize
        );

        // アップロードバッファ作成
        D3D12_HEAP_PROPERTIES uploadHeapProps = {};
        uploadHeapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

        D3D12_RESOURCE_DESC uploadBufferDesc = {};
        uploadBufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        uploadBufferDesc.Width = uploadBufferSize;
        uploadBufferDesc.Height = 1;
        uploadBufferDesc.DepthOrArraySize = 1;
        uploadBufferDesc.MipLevels = 1;
        uploadBufferDesc.Format = DXGI_FORMAT_UNKNOWN;
        uploadBufferDesc.SampleDesc.Count = 1;
        uploadBufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

        ComPtr<ID3D12Resource> uploadBuffer;
        HRESULT hr = device->CreateCommittedResource(
            &uploadHeapProps,
            D3D12_HEAP_FLAG_NONE,
            &uploadBufferDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&uploadBuffer)
        );

        if (FAILED(hr)) {
            throw std::runtime_error("Failed to create texture upload buffer");
        }

        // データをアップロードバッファにコピー
        uint8_t* mappedData = nullptr;
        uploadBuffer->Map(0, nullptr, reinterpret_cast<void**>(&mappedData));

        for (uint32_t i = 0; i < numSubresources; ++i) {
            const D3D12_SUBRESOURCE_DATA& srcData = subresourceData[i];
            const D3D12_PLACED_SUBRESOURCE_FOOTPRINT& layout = layouts[i];

            uint8_t* destSlice = mappedData + layout.Offset;
            const uint8_t* srcSlice = reinterpret_cast<const uint8_t*>(srcData.pData);

            for (uint32_t row = 0; row < numRows[i]; ++row) {
                memcpy(
                    destSlice + row * layout.Footprint.RowPitch,
                    srcSlice + row * srcData.RowPitch,
                    rowSizeInBytes[i]
                );
            }
        }

        uploadBuffer->Unmap(0, nullptr);

        // GPUにコピー
        for (uint32_t i = 0; i < numSubresources; ++i) {
            D3D12_TEXTURE_COPY_LOCATION destLocation = {};
            destLocation.pResource = destinationResource;
            destLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
            destLocation.SubresourceIndex = i;

            D3D12_TEXTURE_COPY_LOCATION srcLocation = {};
            srcLocation.pResource = uploadBuffer.Get();
            srcLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
            srcLocation.PlacedFootprint = layouts[i];

            commandList->CopyTextureRegion(&destLocation, 0, 0, 0, &srcLocation, nullptr);
        }

        // アップロードバッファを保持（End()まで）
        uploadBuffers.push_back(uploadBuffer);
    }

    void UploadContext::TransitionResource(
        ID3D12Resource* resource,
        D3D12_RESOURCE_STATES stateBefore,
        D3D12_RESOURCE_STATES stateAfter) {

        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barrier.Transition.pResource = resource;
        barrier.Transition.StateBefore = stateBefore;
        barrier.Transition.StateAfter = stateAfter;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

        commandList->ResourceBarrier(1, &barrier);
    }

    void UploadContext::End() {
        // コマンドリストを閉じる
        commandList->Close();

        // コマンドキューに送信
        ID3D12CommandList* cmdLists[] = { commandList.Get() };
        commandQueue->ExecuteCommandLists(1, cmdLists);

        // GPU完了を待機
        WaitForGPU();

        // アップロードバッファをクリア（自動削除）
        uploadBuffers.clear();
    }

    void UploadContext::WaitForGPU() {
        ++fenceValue;
        commandQueue->Signal(fence.Get(), fenceValue);

        if (fence->GetCompletedValue() < fenceValue) {
            fence->SetEventOnCompletion(fenceValue, fenceEvent);
            WaitForSingleObject(fenceEvent, INFINITE);
        }
    }

    void UploadContext::ResetCommandList() {
        commandAllocator->Reset();
        commandList->Reset(commandAllocator.Get(), nullptr);
    }

} // namespace Athena