#pragma once

#include <Windows.h>
#include <d3d12.h>
#include <wrl/client.h>
#include <memory>
#include <chrono>
#include <array>

namespace Athena {

    using Microsoft::WRL::ComPtr;

    /**
     * ImGUI integration manager class
     * 
     * Manages ImGUI initialization, rendering, and event handling
     */
    class ImGuiManager {
    public:
        ImGuiManager();
        ~ImGuiManager();

        /**
         * Initialize ImGUI
         * @param hwnd Window handle
         * @param device DirectX 12 device
         * @param numFrames Swap chain buffer count
         * @param rtvFormat Render target format
         * @param srvDescriptorHeap SRV descriptor heap
         */
        bool Initialize(
            HWND hwnd,
            ID3D12Device* device,
            ID3D12CommandQueue* commandQueue,
            int numFrames,
            DXGI_FORMAT rtvFormat,
            ID3D12DescriptorHeap* srvDescriptorHeap
        );

        /**
         * Release resources
         */
        void Shutdown();

        /**
         * Begin frame processing
         */
        void BeginFrame();

        /**
         * End frame and rendering processing
         * @param commandList Rendering command list
         */
        void EndFrame(ID3D12GraphicsCommandList* commandList);

        /**
         * Message processing to be called from window procedure
         */
        bool ProcessWin32Message(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

        /**
         * Render debug UI
         */
        void RenderDebugUI();

        // UI state getters
        bool IsRenderingModeChanged() const { return renderingModeChanged; }
        bool IsDeferredRenderingEnabled() const { return enableDeferredRendering; }
        
        bool IsCameraResetRequested() const { return cameraResetRequested; }
        bool IsMouseCaptureChanged() const { return mouseCaptureChanged; }
        bool IsMouseCaptured() const { return mouseCaptured; }
        
        void ResetChangeFlags() { 
            renderingModeChanged = false; 
            cameraResetRequested = false;
            mouseCaptureChanged = false;
        }

        // Performance information update
        void UpdatePerformanceStats(float deltaTime);
        
        // Rendering statistics update
        void UpdateRenderingStats(uint32_t drawCalls, uint32_t vertexCount, float memoryUsageMB);
        void UpdateRenderGraphStats(uint32_t totalPasses, uint32_t totalResources, uint32_t executedPasses, float renderGraphMemoryMB);

    private:
        bool initialized;
        
        // UI control state
        bool enableDeferredRendering;
        bool renderingModeChanged;
        bool cameraResetRequested;
        bool mouseCaptured;
        bool mouseCaptureChanged;
        
        // Performance statistics
        float currentFPS;
        float frameTime;
        float averageFPS;
        
        // Frame time history (for average FPS calculation)
        std::array<float, 60> frameTimeHistory;
        int frameTimeIndex;
        
        // Camera control UI
        bool showCameraControls;
        
        // RenderGraph statistics
        bool showRenderGraphStats;
        
        // Rendering statistics
        uint32_t drawCallCount;
        uint32_t vertexCount;
        float memoryUsageMB;
        
        // RenderGraph statistics
        uint32_t renderGraphTotalPasses;
        uint32_t renderGraphTotalResources;
        uint32_t renderGraphExecutedPasses;
        float renderGraphMemoryMB;
        
        /**
         * Render camera control UI
         */
        void RenderCameraControls();
        
        /**
         * Render performance statistics UI
         */
        void RenderPerformanceStats();
        
        /**
         * Render rendering settings UI
         */
        void RenderRenderingSettings();
    };

} // namespace Athena