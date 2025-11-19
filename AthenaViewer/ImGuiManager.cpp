#include "ImGuiManager.h"
#include "../external/imgui/imgui.h"
#include "../external/imgui/imgui_impl_win32.h"
#include "../external/imgui/imgui_impl_dx12.h"

namespace Athena {

    ImGuiManager::ImGuiManager() {
        frameTimeHistory.fill(16.67f); // Initialize to 60FPS equivalent
        frameTimeIndex = 0;
        initialized = false;
        enableDeferredRendering = false;
        currentGeometry = 0;
        renderingModeChanged = false;
        geometryModeChanged = false;
        cameraResetRequested = false;
        currentFPS = 0.0f;
        frameTime = 0.0f;
        averageFPS = 60.0f;
        showCameraControls = true;
        showRenderGraphStats = true;
    }

    ImGuiManager::~ImGuiManager() {
        Shutdown();
    }

    bool ImGuiManager::Initialize(
        HWND hwnd,
        ID3D12Device* device,
        ID3D12CommandQueue* commandQueue,
        int numFrames,
        DXGI_FORMAT rtvFormat,
        ID3D12DescriptorHeap* srvDescriptorHeap
    ) {
        // Create ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable keyboard navigation

        // Set style (dark theme)
        ImGui::StyleColorsDark();

        // Initialize platform/renderer bindings
        if (!ImGui_ImplWin32_Init(hwnd)) {
            return false;
        }

        // Use ImGUI DX12 new API with proper initialization
        ImGui_ImplDX12_InitInfo initInfo = {};
        initInfo.Device = device;
        initInfo.CommandQueue = commandQueue;
        initInfo.NumFramesInFlight = numFrames;
        initInfo.RTVFormat = rtvFormat;
        initInfo.SrvDescriptorHeap = srvDescriptorHeap;
        
        // Allocate descriptor handles for font texture
        D3D12_CPU_DESCRIPTOR_HANDLE fontSrvCpuDescHandle = srvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
        D3D12_GPU_DESCRIPTOR_HANDLE fontSrvGpuDescHandle = srvDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
        
        // Move to next descriptor slot for font (assuming first slot is used for font)
        UINT descriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        fontSrvCpuDescHandle.ptr += descriptorSize * 2; // Use slot 2 for ImGUI font
        fontSrvGpuDescHandle.ptr += descriptorSize * 2;
        
        initInfo.LegacySingleSrvCpuDescriptor = fontSrvCpuDescHandle;
        initInfo.LegacySingleSrvGpuDescriptor = fontSrvGpuDescHandle;
        
        if (!ImGui_ImplDX12_Init(&initInfo)) {
            ImGui_ImplWin32_Shutdown();
            return false;
        }

        initialized = true;
        return true;
    }

    void ImGuiManager::Shutdown() {
        if (initialized) {
            ImGui_ImplDX12_Shutdown();
            ImGui_ImplWin32_Shutdown();
            ImGui::DestroyContext();
            initialized = false;
        }
    }

    void ImGuiManager::BeginFrame() {
        if (!initialized) return;

        // Start new frame from backends
        ImGui_ImplDX12_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
    }

    void ImGuiManager::EndFrame(ID3D12GraphicsCommandList* commandList) {
        if (!initialized) return;

        // Render UI
        RenderDebugUI();

        // Render
        ImGui::Render();
        ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList);
    }

    bool ImGuiManager::ProcessWin32Message(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        if (!initialized) return false;

        // ImGui message processing function (declared externally)
        extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
        return ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam) != 0;
    }

    void ImGuiManager::UpdatePerformanceStats(float deltaTime) {
        frameTime = deltaTime * 1000.0f; // Convert to milliseconds
        currentFPS = 1.0f / deltaTime;

        // Update frame time history
        frameTimeHistory[frameTimeIndex] = frameTime;
        frameTimeIndex = (frameTimeIndex + 1) % frameTimeHistory.size();

        // Calculate average FPS
        float totalFrameTime = 0.0f;
        for (float time : frameTimeHistory) {
            totalFrameTime += time;
        }
        averageFPS = 1000.0f * frameTimeHistory.size() / totalFrameTime;
    }

    void ImGuiManager::RenderDebugUI() {
        // Main debug window
        if (ImGui::Begin("Athena Renderer Debug")) {
            RenderRenderingSettings();
            ImGui::Separator();
            RenderPerformanceStats();
            ImGui::Separator();
            RenderCameraControls();
        }
        ImGui::End();

        // RenderGraph statistics (separate window)
        if (showRenderGraphStats) {
            if (ImGui::Begin("RenderGraph Statistics", &showRenderGraphStats)) {
                ImGui::Text("RenderGraph Debug Info");
                ImGui::Text("Total Passes: N/A"); // TODO: Get from RenderGraph
                ImGui::Text("Total Resources: N/A");
                ImGui::Text("Executed Passes: N/A");
                ImGui::Text("Memory Usage: N/A MB");
            }
            ImGui::End();
        }
    }

    void ImGuiManager::RenderRenderingSettings() {
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 1.0f, 1.0f), "Rendering Settings");

        // Rendering mode toggle
        bool oldDeferredMode = enableDeferredRendering;
        if (ImGui::Checkbox("Deferred Rendering", &enableDeferredRendering)) {
            renderingModeChanged = true;
        }
        ImGui::SameLine();
        ImGui::TextDisabled("(?)");
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Toggle between Forward and Deferred rendering modes\n"
                            "Forward: Direct lighting calculation\n"
                            "Deferred: G-Buffer based lighting (better for many lights)");
        }

        // Geometry toggle
        const char* geometryItems[] = { "Sphere", "Cube" }; // 順序を逆にして修正
        int oldGeometry = currentGeometry;
        if (ImGui::Combo("Geometry", &currentGeometry, geometryItems, IM_ARRAYSIZE(geometryItems))) {
            geometryModeChanged = true;
        }

        // Current rendering state display
        ImGui::Spacing();
        ImGui::Text("Current Mode: %s", enableDeferredRendering ? "Deferred" : "Forward");
        // 実際のジオメトリモード表示（DisplayModeに基づく）
        ImGui::Text("Current Geometry: %s", (currentGeometry == 0) ? "Cube" : "Sphere");
    }

    void ImGuiManager::RenderPerformanceStats() {
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Performance");

        // FPS display
        ImGui::Text("FPS: %.1f (%.3f ms)", currentFPS, frameTime);
        ImGui::Text("Avg FPS: %.1f", averageFPS);

        // Frame time graph
        ImGui::PlotLines("Frame Times", frameTimeHistory.data(), frameTimeHistory.size(),
            frameTimeIndex, "Frame Time (ms)", 0.0f, 50.0f, ImVec2(0, 80));

        // Memory usage (placeholder)
        ImGui::Text("Memory Usage: %.1f MB", 0.0f); // TODO: Get actual memory usage

        // GPU statistics (placeholder)
        ImGui::Text("Draw Calls: %d", 0); // TODO: Get actual draw call count
        ImGui::Text("Vertices: %d", 0); // TODO: Get actual vertex count
    }

    void ImGuiManager::RenderCameraControls() {
        if (!showCameraControls) return;

        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Camera Controls");
        
        ImGui::TextWrapped("Controls:");
        ImGui::BulletText("WASD: Move camera");
        ImGui::BulletText("Mouse: Look around (when captured)");
        ImGui::BulletText("C: Toggle mouse capture");
        ImGui::BulletText("Mouse Wheel: Move forward/backward");
        
        // Camera reset button
        if (ImGui::Button("Reset Camera")) {
            cameraResetRequested = true;
        }
        
        ImGui::Spacing();
        
        // Help information toggle
        if (ImGui::CollapsingHeader("Legacy Keyboard Controls")) {
            ImGui::TextWrapped("Legacy controls (still available):");
            ImGui::BulletText("G: Toggle rendering mode");
            ImGui::BulletText("Space: Switch geometry");
            ImGui::BulletText("ESC: Exit application");
        }
    }

} // namespace Athena