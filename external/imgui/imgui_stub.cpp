// ImGUI スタブ実装
// 実際の使用時は公式のImGUIライブラリに置き換えてください

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx12.h"

// グローバル状態（実際のImGUIはより複雑）
static bool g_initialized = false;
static ImGuiIO g_io;

namespace ImGui {
    ImGuiContext* CreateContext(void*) {
        return (ImGuiContext*)1; // ダミーポインタ
    }
    
    void DestroyContext(ImGuiContext*) {
        // スタブ実装
    }
    
    ImGuiIO& GetIO() {
        return g_io;
    }
    
    void NewFrame() {
        // スタブ実装
    }
    
    void Render() {
        // スタブ実装
    }
    
    bool Begin(const char*, bool*, int) {
        return true; // 常にtrueを返す（スタブ）
    }
    
    void End() {
        // スタブ実装
    }
    
    void Text(const char*, ...) {
        // スタブ実装
    }
    
    void TextColored(const ImVec4&, const char*, ...) {
        // スタブ実装
    }
    
    void TextDisabled(const char*, ...) {
        // スタブ実装
    }
    
    void TextWrapped(const char*, ...) {
        // スタブ実装
    }
    
    bool Button(const char*, const ImVec2&) {
        return false; // スタブ実装
    }
    
    bool Checkbox(const char*, bool*) {
        return false; // スタブ実装
    }
    
    bool Combo(const char*, int*, const char* const*, int, int) {
        return false; // スタブ実装
    }
    
    void Separator() {
        // スタブ実装
    }
    
    void Spacing() {
        // スタブ実装
    }
    
    void SameLine(float, float) {
        // スタブ実装
    }
    
    void BulletText(const char*, ...) {
        // スタブ実装
    }
    
    bool CollapsingHeader(const char*, int) {
        return false; // スタブ実装
    }
    
    bool IsItemHovered(int) {
        return false; // スタブ実装
    }
    
    void SetTooltip(const char*, ...) {
        // スタブ実装
    }
    
    void PlotLines(const char*, const float*, int, int, const char*, float, float, ImVec2, int) {
        // スタブ実装
    }
    
    void StyleColorsDark() {
        // スタブ実装
    }
}

// Win32実装
bool ImGui_ImplWin32_Init(void*) {
    g_initialized = true;
    return true;
}

void ImGui_ImplWin32_Shutdown() {
    g_initialized = false;
}

void ImGui_ImplWin32_NewFrame() {
    // スタブ実装
}

LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM) {
    return 0; // スタブ実装
}

// DX12実装
bool ImGui_ImplDX12_Init(ID3D12Device*, int, DXGI_FORMAT, ID3D12DescriptorHeap*, D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE) {
    return true; // スタブ実装
}

void ImGui_ImplDX12_Shutdown() {
    // スタブ実装
}

void ImGui_ImplDX12_NewFrame() {
    // スタブ実装
}

void ImGui_ImplDX12_RenderDrawData(ImDrawData*, ID3D12GraphicsCommandList*) {
    // スタブ実装
}

// 描画データ（ダミー）
ImDrawData* ImGui::GetDrawData() {
    static ImDrawData dummy;
    return &dummy;
}