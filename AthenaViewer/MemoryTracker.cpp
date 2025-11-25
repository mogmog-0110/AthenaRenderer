#include "MemoryTracker.h"

namespace Athena {

    MemoryTracker::MemoryTracker() 
        : totalBufferMemory(0), totalTextureMemory(0) {
        InitializeDXGIAdapter();
    }

    MemoryTracker::~MemoryTracker() {
    }

    bool MemoryTracker::InitializeDXGIAdapter() {
        ComPtr<IDXGIFactory4> factory;
        HRESULT hr = CreateDXGIFactory2(0, IID_PPV_ARGS(&factory));
        if (FAILED(hr)) {
            return false;
        }

        ComPtr<IDXGIAdapter1> adapter1;
        hr = factory->EnumAdapters1(0, &adapter1);
        if (FAILED(hr)) {
            return false;
        }

        hr = adapter1.As(&dxgiAdapter);
        return SUCCEEDED(hr);
    }

    void MemoryTracker::AddBufferMemory(uint64_t sizeInBytes) {
        totalBufferMemory += sizeInBytes;
    }

    void MemoryTracker::AddTextureMemory(uint64_t sizeInBytes) {
        totalTextureMemory += sizeInBytes;
    }

    void MemoryTracker::RemoveMemory(uint64_t sizeInBytes) {
        uint64_t currentBuffer = totalBufferMemory.load();
        uint64_t currentTexture = totalTextureMemory.load();
        
        if (sizeInBytes <= currentBuffer) {
            totalBufferMemory -= sizeInBytes;
        } else if (sizeInBytes <= currentTexture) {
            totalTextureMemory -= sizeInBytes;
        } else {
            // 分割して減算
            if (currentBuffer > 0) {
                totalBufferMemory = 0;
                sizeInBytes -= currentBuffer;
            }
            if (sizeInBytes <= currentTexture) {
                totalTextureMemory -= sizeInBytes;
            }
        }
    }

    float MemoryTracker::GetD3D12MemoryUsageMB() const {
        uint64_t total = totalBufferMemory.load() + totalTextureMemory.load();
        return static_cast<float>(total) / (1024.0f * 1024.0f);
    }

    float MemoryTracker::GetProcessMemoryUsageMB() const {
        PROCESS_MEMORY_COUNTERS_EX pmc;
        if (GetProcessMemoryInfo(GetCurrentProcess(), 
                               reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&pmc), 
                               sizeof(pmc))) {
            return static_cast<float>(pmc.WorkingSetSize) / (1024.0f * 1024.0f);
        }
        return 0.0f;
    }

    float MemoryTracker::GetGPUMemoryUsageMB() const {
        if (!dxgiAdapter) {
            return 0.0f;
        }

        DXGI_QUERY_VIDEO_MEMORY_INFO videoMemoryInfo;
        HRESULT hr = dxgiAdapter->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &videoMemoryInfo);
        
        if (SUCCEEDED(hr)) {
            uint64_t usedMemory = videoMemoryInfo.CurrentUsage;
            return static_cast<float>(usedMemory) / (1024.0f * 1024.0f);
        }

        return 0.0f;
    }

    void MemoryTracker::Reset() {
        totalBufferMemory = 0;
        totalTextureMemory = 0;
    }

} // namespace Athena