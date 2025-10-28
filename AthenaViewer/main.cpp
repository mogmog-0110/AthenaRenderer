#include <Windows.h>
#include <exception>
#include <memory>
#include <stdexcept>

// Athena includes
#include "../AthenaCore/include/Athena/Utils/Logger.h"
#include "../AthenaCore/include/Athena/Core/Device.h"
#include "../AthenaCore/include/Athena/Core/CommandQueue.h"
#include "../AthenaCore/include/Athena/Core/SwapChain.h"
#include "../AthenaCore/include/Athena/Core/DescriptorHeap.h"

using namespace Athena;

// グローバル変数
HWND g_hwnd = nullptr;
constexpr uint32_t g_WindowWidth = 1280;
constexpr uint32_t g_WindowHeight = 720;

// DirectX 12オブジェクト
std::unique_ptr<Device> g_device;
std::unique_ptr<CommandQueue> g_commandQueue;
std::unique_ptr<SwapChain> g_swapChain;
std::unique_ptr<DescriptorHeap> g_rtvHeap;

// コマンド記録用
ComPtr<ID3D12CommandAllocator> g_commandAllocator;
ComPtr<ID3D12GraphicsCommandList> g_commandList;

// レンダーターゲットビュー
DescriptorHandle g_rtvHandles[SwapChain::BufferCount];

// 前方宣言
LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
void InitializeDirectX();
void CreateRenderTargetViews();
void Render();
void Cleanup();

/**
 * @brief Windowsアプリケーションのエントリーポイント
 */
int WINAPI WinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPSTR lpCmdLine,
    _In_ int nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    try {
        // ログシステム初期化
        Logger::Initialize();
        Logger::Info("===========================================");
        Logger::Info("Athena Renderer - Phase 1: Clear Test");
        Logger::Info("===========================================");

        // ウィンドウクラスの登録
        WNDCLASSEXW wc = {};
        wc.cbSize = sizeof(WNDCLASSEXW);
        wc.style = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc = WindowProc;
        wc.hInstance = hInstance;
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.lpszClassName = L"AthenaRendererClass";

        if (!RegisterClassExW(&wc)) {
            throw std::runtime_error("Failed to register window class");
        }

        // ウィンドウ作成
        RECT rect = { 0, 0, static_cast<LONG>(g_WindowWidth), static_cast<LONG>(g_WindowHeight) };
        AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);

        g_hwnd = CreateWindowExW(
            0,
            L"AthenaRendererClass",
            L"Athena Renderer - Phase 1: Clear Test",
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT, CW_USEDEFAULT,
            rect.right - rect.left,
            rect.bottom - rect.top,
            nullptr,
            nullptr,
            hInstance,
            nullptr
        );

        if (!g_hwnd) {
            throw std::runtime_error("Failed to create window");
        }

        // DirectX 12初期化
        InitializeDirectX();

        // ウィンドウ表示
        ShowWindow(g_hwnd, nCmdShow);
        UpdateWindow(g_hwnd);

        Logger::Info("Initialization complete. Entering main loop...");

        // メインループ
        MSG msg = {};
        bool running = true;

        while (running) {
            // メッセージ処理
            while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
                if (msg.message == WM_QUIT) {
                    running = false;
                    break;
                }
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }

            if (running) {
                // レンダリング
                Render();
            }
        }

        // 終了処理
        Cleanup();
        Logger::Info("Application shutting down normally");
        Logger::Shutdown();

        return static_cast<int>(msg.wParam);
    }
    catch (const std::exception& e) {
        // エラー発生時
        Logger::Error("Fatal error: %s", e.what());
        MessageBoxA(nullptr, e.what(), "Error", MB_OK | MB_ICONERROR);
        Logger::Shutdown();
        return 1;
    }
}

/**
 * @brief DirectX 12の初期化
 */
void InitializeDirectX() {
    Logger::Info("Initializing DirectX 12...");

    // Device初期化
    g_device = std::make_unique<Device>();
    g_device->Initialize(true);  // デバッグレイヤー有効

    // CommandQueue初期化
    g_commandQueue = std::make_unique<CommandQueue>();
    g_commandQueue->Initialize(g_device->GetD3D12Device(), D3D12_COMMAND_LIST_TYPE_DIRECT);

    // SwapChain初期化
    g_swapChain = std::make_unique<SwapChain>();
    g_swapChain->Initialize(
        g_device->GetDXGIFactory(),
        g_commandQueue->GetD3D12CommandQueue(),
        g_hwnd,
        g_WindowWidth,
        g_WindowHeight
    );

    // RTV用デスクリプタヒープ作成
    g_rtvHeap = std::make_unique<DescriptorHeap>();
    g_rtvHeap->Initialize(
        g_device->GetD3D12Device(),
        D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
        SwapChain::BufferCount,
        false  // シェーダー不可視
    );

    // CommandAllocator作成
    HRESULT hr = g_device->GetD3D12Device()->CreateCommandAllocator(
        D3D12_COMMAND_LIST_TYPE_DIRECT,
        IID_PPV_ARGS(&g_commandAllocator)
    );
    if (FAILED(hr)) {
        throw std::runtime_error("Failed to create command allocator");
    }

    // CommandList作成
    hr = g_device->GetD3D12Device()->CreateCommandList(
        0,
        D3D12_COMMAND_LIST_TYPE_DIRECT,
        g_commandAllocator.Get(),
        nullptr,
        IID_PPV_ARGS(&g_commandList)
    );
    if (FAILED(hr)) {
        throw std::runtime_error("Failed to create command list");
    }

    // コマンドリストを一旦閉じる（初期状態で開いているため）
    g_commandList->Close();

    // レンダーターゲットビュー作成
    CreateRenderTargetViews();

    Logger::Info("DirectX 12 initialized successfully");
}

/**
 * @brief レンダーターゲットビューの作成
 */
void CreateRenderTargetViews() {
    Logger::Info("Creating Render Target Views...");

    for (uint32_t i = 0; i < SwapChain::BufferCount; ++i) {
        // デスクリプタを割り当て
        g_rtvHandles[i] = g_rtvHeap->Allocate();

        // レンダーターゲットビューを作成
        ID3D12Resource* backBuffer = g_swapChain->GetBackBuffer(i);
        g_device->GetD3D12Device()->CreateRenderTargetView(
            backBuffer,
            nullptr,
            g_rtvHandles[i].cpu
        );
    }

    Logger::Info("Render Target Views created");
}

/**
 * @brief レンダリング処理
 */
void Render() {
    // コマンドアロケーターをリセット
    g_commandAllocator->Reset();

    // コマンドリストをリセット
    g_commandList->Reset(g_commandAllocator.Get(), nullptr);

    // 現在のバックバッファインデックス取得
    uint32_t backBufferIndex = g_swapChain->GetCurrentBackBufferIndex();
    ID3D12Resource* backBuffer = g_swapChain->GetCurrentBackBuffer();

    // リソースバリア: Present → RenderTarget
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource = backBuffer;
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

    g_commandList->ResourceBarrier(1, &barrier);

    // 画面クリア（青色）
    float clearColor[4] = { 0.2f, 0.4f, 0.8f, 1.0f };  // R, G, B, A
    g_commandList->ClearRenderTargetView(
        g_rtvHandles[backBufferIndex].cpu,
        clearColor,
        0,
        nullptr
    );

    // リソースバリア: RenderTarget → Present
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
    g_commandList->ResourceBarrier(1, &barrier);

    // コマンドリストを閉じる
    g_commandList->Close();

    // コマンドリストを実行
    ID3D12CommandList* commandLists[] = { g_commandList.Get() };
    g_commandQueue->ExecuteCommandLists(commandLists, 1);

    // 画面に表示
    g_swapChain->Present(true);  // VSync有効

    // GPUの完了を待つ
    g_commandQueue->WaitForGPU();
}

/**
 * @brief 終了処理
 */
void Cleanup() {
    Logger::Info("Cleaning up resources...");

    // GPU待機
    if (g_commandQueue) {
        g_commandQueue->WaitForGPU();
    }

    // RTVの解放
    for (uint32_t i = 0; i < SwapChain::BufferCount; ++i) {
        if (g_rtvHeap && g_rtvHandles[i].IsValid()) {
            g_rtvHeap->Free(g_rtvHandles[i]);
        }
    }

    // リソース解放
    g_commandList.Reset();
    g_commandAllocator.Reset();

    if (g_rtvHeap) g_rtvHeap->Shutdown();
    if (g_swapChain) g_swapChain->Shutdown();
    if (g_commandQueue) g_commandQueue->Shutdown();
    if (g_device) g_device->Shutdown();

    Logger::Info("Cleanup complete");
}

/**
 * @brief ウィンドウプロシージャ
 */
LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE) {
            PostQuitMessage(0);
        }
        return 0;
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}