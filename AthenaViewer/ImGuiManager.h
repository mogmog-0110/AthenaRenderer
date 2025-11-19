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
        
        bool IsGeometryModeChanged() const { return geometryModeChanged; }
        int GetCurrentGeometryMode() const { return currentGeometry; }
        
        bool IsCameraResetRequested() const { return cameraResetRequested; }
        
        void ResetChangeFlags() { 
            renderingModeChanged = false; 
            geometryModeChanged = false; 
            cameraResetRequested = false;
        }

        // Performance information update
        void UpdatePerformanceStats(float deltaTime);

    private:
        bool initialized;
        
        // UI control state
        bool enableDeferredRendering;
        int currentGeometry; // 0: Cube, 1: Sphere
        bool renderingModeChanged;
        bool geometryModeChanged;
        bool cameraResetRequested;
        
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