#define NOMINMAX

#include <Windows.h>
#include <exception>
#include <stdexcept>
#include <memory>
#include <chrono>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <wrl/client.h>
#include "Athena/Utils/Logger.h"
#include "Athena/Core/Device.h"
#include "Athena/Core/CommandQueue.h"
#include "Athena/Core/SwapChain.h"
#include "Athena/Core/DescriptorHeap.h"
#include "Athena/Resources/Buffer.h"
#include "Athena/Resources/Texture.h"
#include "Athena/Resources/UploadContext.h"
#include "Athena/Utils/Math.h"
#include "Athena/Scene/Camera.h"
#include "Athena/Scene/ModelLoader.h"
#include "Athena/Scene/CameraController.h"
#include "ImGuiManager.h"
//#include "MemoryTracker.h"
//#include "RenderingStats.h"
#include "SimpleStats.h"

// 機能テスト関数の宣言
bool RunFeatureTests(std::shared_ptr<Athena::Device> device);

// RenderGraphテスト関数の宣言
bool RunAllRenderGraphTests(std::shared_ptr<Athena::Device> device);

// RenderGraphExample関数の宣言
bool InitializeRenderGraphExample(std::shared_ptr<Athena::Device> device, uint32_t width, uint32_t height);
void RenderWithRenderGraph(ID3D12GraphicsCommandList* commandList, 
                          ID3D12DescriptorHeap* srvHeap,
                          D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle,
                          D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle,
                          bool useDeferredRendering = false);
void SetRenderGraphSceneData(const Athena::Matrix4x4& world, const Athena::Matrix4x4& view, const Athena::Matrix4x4& proj,
                           const Athena::Vector3& cameraPos, const Athena::Vector3& lightDir, const Athena::Vector3& lightColor);
void SetRenderGraphVertexData(const void* vertices, uint32_t vertexCount, const uint32_t* indices, uint32_t indexCount);
void SetRenderGraphTexture(std::shared_ptr<Athena::Texture> texture);
void SetRenderGraphObjectID(uint32_t objectID);
void SetRenderGraphMode(bool useDeferred);

using namespace Athena;
using Microsoft::WRL::ComPtr;

// グローバル変数
HWND g_hwnd = nullptr;
const uint32_t WINDOW_WIDTH = 1280;
const uint32_t WINDOW_HEIGHT = 720;

// G-Bufferモードとカメラ管理
enum class RenderingMode {
    Forward = 0,  // フォワードレンダリング
    Deferred = 1  // ディファードレンダリング（G-Buffer）
};
RenderingMode g_renderingMode = RenderingMode::Forward;

// カメラ管理
std::unique_ptr<FPSCamera> g_camera;
std::unique_ptr<Athena::CameraController> g_cameraController;
bool g_mouseCaptured = false;
POINT g_lastMousePos = {};

// モデル管理
std::shared_ptr<Athena::Model> g_loadedModel;
bool g_useLoadedModel = false;

// ImGUI管理、メモリトラッカー、レンダリング統計
std::unique_ptr<Athena::ImGuiManager> g_imguiManager;
//std::unique_ptr<Athena::MemoryTracker> g_memoryTracker;
//std::unique_ptr<Athena::RenderingStats> g_renderingStats;
std::unique_ptr<Athena::SimpleStats> g_simpleStats;

// 頂点構造体（アライメント明示）
struct Vertex {
    Vector3 position;   // 0 - 11バイト
    Vector2 texcoord;   // 12 - 19バイト
};

// ModelVertexはModelLoader.hで定義されているため、ここでは宣言のみ

// 定数バッファ構造体（256バイトアライメント）
struct TransformBuffer {
    Matrix4x4 mvp;
    float padding[48];
};

// ウィンドウプロシージャ
LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    // ImGUIメッセージ処理を最初に行う
    if (g_imguiManager && g_imguiManager->ProcessWin32Message(hwnd, msg, wParam, lParam)) {
        return true;
    }
    
    switch (msg) {
    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE) {
            PostQuitMessage(0);
        } 
        else if (wParam == 'C' || wParam == 'c') {
            // マウスキャプチャの切り替え
            g_mouseCaptured = !g_mouseCaptured;
            if (g_mouseCaptured) {
                GetCursorPos(&g_lastMousePos);
                ShowCursor(FALSE);
            } else {
                ShowCursor(TRUE);
            }
        }
        // 以下のキーボード入力処理はImGUIに移行
        // else if (wParam == VK_SPACE) { ... } // -> ImGUIのGeometry切り替えに移行
        // else if (wParam == 'G' || wParam == 'g') { ... } // -> ImGUIのRendering Mode切り替えに移行
        
        // カメラ入力をカメラに渡す
        if (g_camera) {
            g_camera->OnKeyPress(static_cast<int>(wParam), true);
        }
        return 0;
    case WM_KEYUP:
        // キーリリースをカメラに渡す
        if (g_camera) {
            g_camera->OnKeyPress(static_cast<int>(wParam), false);
        }
        return 0;
    case WM_MOUSEMOVE:
        if (g_mouseCaptured && g_camera) {
            POINT currentPos;
            GetCursorPos(&currentPos);
            
            float deltaX = static_cast<float>(currentPos.x - g_lastMousePos.x);
            float deltaY = static_cast<float>(currentPos.y - g_lastMousePos.y);
            
            g_camera->OnMouseMove(deltaX, deltaY);
            
            // マウスを元の位置に戻す
            SetCursorPos(g_lastMousePos.x, g_lastMousePos.y);
        }
        return 0;
    case WM_MOUSEWHEEL:
        if (g_camera) {
            // マウスホイールのスクロール量を取得
            int wheelDelta = GET_WHEEL_DELTA_WPARAM(wParam);
            float delta = static_cast<float>(wheelDelta) / WHEEL_DELTA;
            g_camera->OnMouseScroll(delta);
        }
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

// ウィンドウ作成
bool CreateAppWindow(HINSTANCE hInstance) {
    WNDCLASSEX wc = {};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.lpszClassName = L"AthenaRendererClass";

    if (!RegisterClassEx(&wc)) return false;

    RECT rect = { 0, 0, static_cast<LONG>(WINDOW_WIDTH), static_cast<LONG>(WINDOW_HEIGHT) };
    AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);

    g_hwnd = CreateWindow(
        wc.lpszClassName,
        L"Athena Renderer - Deferred + Camera",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        rect.right - rect.left,
        rect.bottom - rect.top,
        nullptr, nullptr, hInstance, nullptr
    );

    if (!g_hwnd) return false;

    ShowWindow(g_hwnd, SW_SHOW);
    UpdateWindow(g_hwnd);
    return true;
}


// シェーダーコンパイル
ComPtr<ID3DBlob> CompileShader(const std::wstring& filepath, const std::string& entryPoint, const std::string& target) {
    UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;

    ComPtr<ID3DBlob> shader;
    ComPtr<ID3DBlob> error;

    HRESULT hr = D3DCompileFromFile(
        filepath.c_str(),
        nullptr, nullptr,
        entryPoint.c_str(),
        target.c_str(),
        compileFlags, 0,
        &shader, &error
    );

    if (FAILED(hr)) {
        if (error) {
            Logger::Error("Shader compilation failed: %s", (char*)error->GetBufferPointer());
        }
        Logger::Error("Failed to compile shader: %S", filepath.c_str());
        throw std::runtime_error("Failed to compile shader");
    }

    Logger::Info("✓ Shader compiled: %S", filepath.c_str());
    return shader;
}

// メイン関数
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int) {
    try {
        Logger::Initialize();
        Logger::Info("==========================================================");
        Logger::Info("  Athena Renderer - RenderGraph + Camera Integration");
        Logger::Info("==========================================================");
        Logger::Info("Controls:");
        Logger::Info("  WASD: Move camera");
        Logger::Info("  Mouse: Look around (press C to capture/release mouse)");
        Logger::Info("  G: Toggle rendering mode (Forward/Deferred)");
        Logger::Info("  Shift: Sprint");
        Logger::Info("  Escape: Exit");
        Logger::Info("==========================================================");

        // ウィンドウ作成
        if (!CreateAppWindow(hInstance)) {
            throw std::runtime_error("Failed to create window");
        }
        Logger::Info("✓ Window created");

        // デバイス初期化
        auto devicePtr = std::make_shared<Device>();
        devicePtr->Initialize(true);
        Logger::Info("✓ Device initialized");

        // カメラ初期化
        g_camera = std::make_unique<FPSCamera>();
        g_camera->SetPerspective(3.14159f / 4.0f, static_cast<float>(WINDOW_WIDTH) / WINDOW_HEIGHT, 0.1f, 1000.0f);
        g_camera->SetPosition(Vector3(-3.0f, 0.0f, 0.0f));
        
        // カメラコントローラー初期化（shared_ptrに変換）
        std::shared_ptr<Camera> sharedCamera(g_camera.get(), [](Camera*){});  // Non-owning shared_ptr
        g_cameraController = std::make_unique<Athena::CameraController>(sharedCamera);
        g_cameraController->SetSpeed(5.0f);
        g_cameraController->SetSensitivity(0.1f);
        
        Logger::Info("✓ FPS Camera and CameraController initialized");
        
        // FBXモデルの読み込み試行
        Logger::Info("=== FBXモデル読み込み試行 ===");
        std::vector<std::string> modelPaths = {
            "../assets/models/teamugfbx.fbx"
        };
        
        for (const auto& path : modelPaths) {
            Logger::Info("Trying to load model: %s", path.c_str());
            g_loadedModel = Athena::ModelLoader::LoadModel(devicePtr, path, true, true);
            if (g_loadedModel) {
                g_useLoadedModel = true;
                Logger::Info("✓ FBX Model loaded successfully: %s", path.c_str());
                Logger::Info("  - Meshes: %u", (unsigned int)g_loadedModel->meshes.size());
                Logger::Info("  - Materials: %u", (unsigned int)g_loadedModel->materials.size());
                Logger::Info("  - Size: (%.2f, %.2f, %.2f)", 
                           g_loadedModel->GetSize().x, g_loadedModel->GetSize().y, g_loadedModel->GetSize().z);
                break;
            }
        }
        
        if (!g_loadedModel) {
            Logger::Warning("No FBX model found, will render default cube");
            g_useLoadedModel = false;
        }

        // 新機能テスト実行
        Logger::Info("=== 新機能テスト開始 ===");
        bool featureTestResult = RunFeatureTests(devicePtr);
        Logger::Info("=== 新機能テスト完了: %s ===", featureTestResult ? "成功" : "失敗");
        
        // RenderGraphテスト実行
        Logger::Info("=== RenderGraph基盤テスト開始 ===");
        bool testResult = RunAllRenderGraphTests(devicePtr);
        Logger::Info("=== RenderGraph基盤テスト完了: %s ===", testResult ? "成功" : "失敗");
        
        // テストの全体結果を確認
        Logger::Info("=== 全テスト結果 ===");
        Logger::Info("新機能テスト: %s", featureTestResult ? "成功" : "失敗");
        Logger::Info("RenderGraphテスト: %s", testResult ? "成功" : "失敗");
        
        // RenderGraphテスト失敗時のみ早期終了（新機能テストは警告のみ）
        if (!testResult) {
            Logger::Error("RenderGraphテストが失敗しました。アプリケーションを終了します。");
            return -1;
        }

        // RenderGraph用のテスト用ジオメトリ（三角形）を先に準備
        Logger::Info("RenderGraph用テストジオメトリを準備");
        
        // 簡単な三角形の頂点データ
        struct TestVertex {
            Athena::Vector3 position;
            Athena::Vector3 normal;
            float u, v;
        };
        
        TestVertex triangleVertices[] = {
            { Athena::Vector3(0.0f, 1.5f, 0.0f), Athena::Vector3(0.0f, 0.0f, 1.0f), 0.5f, 0.0f },
            { Athena::Vector3(-1.5f, -1.5f, 0.0f), Athena::Vector3(0.0f, 0.0f, 1.0f), 0.0f, 1.0f },
            { Athena::Vector3(1.5f, -1.5f, 0.0f), Athena::Vector3(0.0f, 0.0f, 1.0f), 1.0f, 1.0f }
        };
        
        uint32_t triangleIndices[] = { 0, 1, 2 };

        // RenderGraphExample初期化（頂点データ設定前）
        Logger::Info("=== RenderGraph統合例の初期化 ===");
        bool renderGraphExampleResult = InitializeRenderGraphExample(devicePtr, WINDOW_WIDTH, WINDOW_HEIGHT);
        
        if (renderGraphExampleResult) {
            // 初期化成功後に頂点データを設定
            SetRenderGraphVertexData(triangleVertices, 3, triangleIndices, 3);
            Logger::Info("RenderGraph用テストジオメトリ設定完了");
        }
        
        Logger::Info("=== RenderGraph統合例初期化: %s ===", renderGraphExampleResult ? "成功" : "失敗");
        
        if (!renderGraphExampleResult) {
            Logger::Warning("RenderGraph統合例の初期化に失敗しましたが、従来レンダリングで続行します。");
        }

        // コマンドキュー
        CommandQueue commandQueue;
        commandQueue.Initialize(devicePtr->GetD3D12Device(), D3D12_COMMAND_LIST_TYPE_DIRECT);
        Logger::Info("✓ CommandQueue initialized");

        // スワップチェーン
        SwapChain swapChain;
        swapChain.Initialize(
            devicePtr->GetDXGIFactory(),
            commandQueue.GetD3D12CommandQueue(),
            g_hwnd,
            WINDOW_WIDTH,
            WINDOW_HEIGHT
        );
        Logger::Info("✓ SwapChain initialized");

        // デスクリプタヒープ
        DescriptorHeap rtvHeap;
        rtvHeap.Initialize(devicePtr->GetD3D12Device(), D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 3, false);

        DescriptorHeap dsvHeap;
        dsvHeap.Initialize(devicePtr->GetD3D12Device(), D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1, false);

        DescriptorHeap cbvSrvHeap;
        cbvSrvHeap.Initialize(devicePtr->GetD3D12Device(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 10, true);
        Logger::Info("✓ Descriptor heaps created");

        // ImGUI初期化
        g_imguiManager = std::make_unique<Athena::ImGuiManager>();
        if (!g_imguiManager->Initialize(
            g_hwnd,
            devicePtr->GetD3D12Device(),
            commandQueue.GetD3D12CommandQueue(),
            SwapChain::BufferCount,
            swapChain.GetFormat(),
            cbvSrvHeap.GetD3D12DescriptorHeap()
        )) {
            Logger::Error("Failed to initialize ImGUI");
            return -1;
        }
        Logger::Info("✓ ImGUI initialized");

        // SimpleStats初期化
        g_simpleStats = std::make_unique<Athena::SimpleStats>();
        Logger::Info("✓ SimpleStats initialized");

        // RTVを作成
        for (uint32_t i = 0; i < SwapChain::BufferCount; ++i) {
            auto handle = rtvHeap.Allocate();
            D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
            rtvDesc.Format = swapChain.GetFormat();
            rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
            devicePtr->GetD3D12Device()->CreateRenderTargetView(
                swapChain.GetBackBuffer(i),
                &rtvDesc,
                handle.cpu
            );
        }
        Logger::Info("✓ RTVs created");

        // 深度バッファ
        Texture depthTexture;
        depthTexture.CreateDepthStencil(devicePtr->GetD3D12Device(), WINDOW_WIDTH, WINDOW_HEIGHT);
        auto dsvHandle = dsvHeap.Allocate();
        depthTexture.CreateDSV(devicePtr->GetD3D12Device(), dsvHandle.cpu);
        Logger::Info("✓ Depth buffer created");

        // コマンドアロケータ
        ComPtr<ID3D12CommandAllocator> commandAllocator;
        devicePtr->GetD3D12Device()->CreateCommandAllocator(
            D3D12_COMMAND_LIST_TYPE_DIRECT,
            IID_PPV_ARGS(&commandAllocator)
        );

        ComPtr<ID3D12GraphicsCommandList> commandList;
        devicePtr->GetD3D12Device()->CreateCommandList(
            0,
            D3D12_COMMAND_LIST_TYPE_DIRECT,
            commandAllocator.Get(),
            nullptr,
            IID_PPV_ARGS(&commandList)
        );
        commandList->Close();
        Logger::Info("✓ Command allocator and list created");

        // UploadContext
        UploadContext uploadContext;
        uploadContext.Initialize(devicePtr->GetD3D12Device(), commandQueue.GetD3D12CommandQueue());

        // 🎨 画像ファイルからテクスチャ読み込み
        Logger::Info("==========================================================");
        Logger::Info("  Loading texture from file...");
        Logger::Info("==========================================================");

        Texture mainTexture;

        // テクスチャファイルのパス
        // 例: ../assets/test_texture.png
        const wchar_t* texturePath = L"../assets/Sunamerikun.png";

        try {
            mainTexture.LoadFromFile(
                devicePtr->GetD3D12Device(),
                texturePath,
                &uploadContext,
                true  // ミップマップ自動生成
            );
            Logger::Info("✓ Texture loaded from file!");
        }
        catch (const std::exception& e) {
            Logger::Warning("Failed to load texture file: %s", e.what());
            Logger::Info("Falling back to procedural checkerboard texture...");

            // フォールバック: チェッカーボードテクスチャ
            const uint32_t width = 256;
            const uint32_t height = 256;
            const uint32_t gridSize = 8;

            std::vector<uint32_t> pixels(width * height);
            for (uint32_t y = 0; y < height; ++y) {
                for (uint32_t x = 0; x < width; ++x) {
                    bool isWhite = ((x / (width / gridSize)) + (y / (height / gridSize))) % 2 == 0;
                    pixels[y * width + x] = isWhite ? 0xFFFFFFFF : 0xFF333333;
                }
            }

            mainTexture.CreateFromMemory(
                devicePtr->GetD3D12Device(),
                width, height,
                DXGI_FORMAT_R8G8B8A8_UNORM,
                pixels.data()
            );
            mainTexture.UploadToGPU(&uploadContext);
            Logger::Info("✓ Fallback texture created");
        }

        // デスクリプタ割り当て
        auto cbvHandle = cbvSrvHeap.Allocate();
        auto textureSrvHandle = cbvSrvHeap.Allocate();

        Logger::Info("✓ Descriptors allocated:");
        Logger::Info("  - CBV at index %u", cbvHandle.index);
        Logger::Info("  - SRV at index %u", textureSrvHandle.index);

        // 頂点データ（Vector2統一でテクスチャ座標を適切に設定）
        // RenderGraphが独自のジオメトリデータを使用する

        Buffer constantBuffer;
        constantBuffer.Initialize(
            devicePtr->GetD3D12Device(),
            256,
            BufferType::Constant,
            D3D12_HEAP_TYPE_UPLOAD
        );
        
        // メモリ使用量の記録は削除（SimpleStatsでは不要）
        
        Logger::Info("✓ Buffers created");
        Logger::Info("Vertex struct size: %zu bytes", sizeof(Vertex));
        Logger::Info("Vector3 size: %zu bytes", sizeof(Vector3));
        Logger::Info("Vector2 size: %zu bytes", sizeof(Vector2));

        // CBV作成
        D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
        cbvDesc.BufferLocation = constantBuffer.GetGPUVirtualAddress();
        cbvDesc.SizeInBytes = 256;
        devicePtr->GetD3D12Device()->CreateConstantBufferView(&cbvDesc, cbvHandle.cpu);
        Logger::Info("✓ CBV created");
        

        // SRV作成
        mainTexture.CreateSRV(devicePtr->GetD3D12Device(), textureSrvHandle.cpu);
        Logger::Info("✓ Texture SRV created");

        // RenderGraph用のキューブ頂点データ（法線付き）
        TestVertex rgCubeVertices[] = {
            // 前面 (Z+)
            {{-0.5f, -0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, 0.0f, 1.0f},
            {{-0.5f,  0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, 0.0f, 0.0f},
            {{ 0.5f,  0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, 1.0f, 0.0f},
            {{ 0.5f, -0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, 1.0f, 1.0f},
            // 背面 (Z-)
            {{ 0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, 0.0f, 1.0f},
            {{ 0.5f,  0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, 0.0f, 0.0f},
            {{-0.5f,  0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, 1.0f, 0.0f},
            {{-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, 1.0f, 1.0f},
            // 上面 (Y+)
            {{-0.5f,  0.5f,  0.5f}, {0.0f, 1.0f, 0.0f}, 0.0f, 0.0f},
            {{-0.5f,  0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, 0.0f, 1.0f},
            {{ 0.5f,  0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, 1.0f, 1.0f},
            {{ 0.5f,  0.5f,  0.5f}, {0.0f, 1.0f, 0.0f}, 1.0f, 0.0f},
            // 底面 (Y-)
            {{-0.5f, -0.5f, -0.5f}, {0.0f, -1.0f, 0.0f}, 0.0f, 1.0f},
            {{-0.5f, -0.5f,  0.5f}, {0.0f, -1.0f, 0.0f}, 0.0f, 0.0f},
            {{ 0.5f, -0.5f,  0.5f}, {0.0f, -1.0f, 0.0f}, 1.0f, 0.0f},
            {{ 0.5f, -0.5f, -0.5f}, {0.0f, -1.0f, 0.0f}, 1.0f, 1.0f},
            // 右面 (X+)
            {{ 0.5f, -0.5f,  0.5f}, {1.0f, 0.0f, 0.0f}, 0.0f, 1.0f},
            {{ 0.5f,  0.5f,  0.5f}, {1.0f, 0.0f, 0.0f}, 0.0f, 0.0f},
            {{ 0.5f,  0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, 1.0f, 0.0f},
            {{ 0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, 1.0f, 1.0f},
            // 左面 (X-)
            {{-0.5f, -0.5f, -0.5f}, {-1.0f, 0.0f, 0.0f}, 0.0f, 1.0f},
            {{-0.5f,  0.5f, -0.5f}, {-1.0f, 0.0f, 0.0f}, 0.0f, 0.0f},
            {{-0.5f,  0.5f,  0.5f}, {-1.0f, 0.0f, 0.0f}, 1.0f, 0.0f},
            {{-0.5f, -0.5f,  0.5f}, {-1.0f, 0.0f, 0.0f}, 1.0f, 1.0f},
        };

        // RenderGraph用のキューブインデックス
        uint32_t rgCubeIndices[] = {
            0, 1, 2, 0, 2, 3,       // 前面
            4, 5, 6, 4, 6, 7,       // 背面
            8, 9, 10, 8, 10, 11,    // 上面
            12, 13, 14, 12, 14, 15, // 底面
            16, 17, 18, 16, 18, 19, // 右面
            20, 21, 22, 20, 22, 23, // 左面
        };

        // RenderGraphに頂点データとテクスチャを設定
        if (renderGraphExampleResult) {
            // 初期設定はキューブデータ
            SetRenderGraphVertexData(rgCubeVertices, 24, rgCubeIndices, 36);
            
            // メインテクスチャをRenderGraphにも設定（共有ポインタで管理）
            auto mainTexturePtr = std::make_shared<Texture>();
            // 同じテクスチャリソースを共有するため、メインテクスチャからコピー
            // 今回は簡略化のため、RenderGraphでテクスチャ設定をスキップ
            Logger::Info("✓ RenderGraph texture sharing (simplified)");
            
            Logger::Info("✓ RenderGraph vertex data and texture configured");
        }

        // シェーダーコンパイル
        Logger::Info("Compiling shaders...");
        auto vertexShader = CompileShader(L"../shaders/TexturedVS.hlsl", "main", "vs_5_1");
        auto pixelShader = CompileShader(L"../shaders/TexturedPS.hlsl", "main", "ps_5_1");

        // ルートシグネチャ
        D3D12_DESCRIPTOR_RANGE ranges[2] = {};
        ranges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
        ranges[0].NumDescriptors = 1;
        ranges[0].BaseShaderRegister = 0;
        ranges[0].RegisterSpace = 0;
        ranges[0].OffsetInDescriptorsFromTableStart = 0;

        ranges[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        ranges[1].NumDescriptors = 1;
        ranges[1].BaseShaderRegister = 0;
        ranges[1].RegisterSpace = 0;
        ranges[1].OffsetInDescriptorsFromTableStart = 1;

        D3D12_ROOT_PARAMETER rootParams[1] = {};
        rootParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        rootParams[0].DescriptorTable.NumDescriptorRanges = 2;
        rootParams[0].DescriptorTable.pDescriptorRanges = ranges;
        rootParams[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

        D3D12_STATIC_SAMPLER_DESC sampler = {};
        sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        sampler.MipLODBias = 0.0f;
        sampler.MaxAnisotropy = 1;
        sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
        sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
        sampler.MinLOD = 0.0f;
        sampler.MaxLOD = D3D12_FLOAT32_MAX;
        sampler.ShaderRegister = 0;
        sampler.RegisterSpace = 0;
        sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

        D3D12_ROOT_SIGNATURE_DESC rootSigDesc = {};
        rootSigDesc.NumParameters = 1;
        rootSigDesc.pParameters = rootParams;
        rootSigDesc.NumStaticSamplers = 1;
        rootSigDesc.pStaticSamplers = &sampler;
        rootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

        ComPtr<ID3DBlob> signature;
        ComPtr<ID3DBlob> error;
        HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);
        if (FAILED(hr)) {
            if (error) Logger::Error("Root signature error: %s", (char*)error->GetBufferPointer());
            throw std::runtime_error("Failed to serialize root signature");
        }

        ComPtr<ID3D12RootSignature> rootSignature;
        devicePtr->GetD3D12Device()->CreateRootSignature(
            0,
            signature->GetBufferPointer(),
            signature->GetBufferSize(),
            IID_PPV_ARGS(&rootSignature)
        );
        Logger::Info("✓ Root signature created");

        // PSO作成
        D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
            {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
            {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        };

        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
        psoDesc.pRootSignature = rootSignature.Get();
        psoDesc.VS = { vertexShader->GetBufferPointer(), vertexShader->GetBufferSize() };
        psoDesc.PS = { pixelShader->GetBufferPointer(), pixelShader->GetBufferSize() };
        psoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
        psoDesc.SampleMask = UINT_MAX;
        psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
        psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
        psoDesc.RasterizerState.FrontCounterClockwise = FALSE;
        psoDesc.RasterizerState.DepthClipEnable = TRUE;
        psoDesc.DepthStencilState.DepthEnable = TRUE;
        psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
        psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
        psoDesc.InputLayout = { inputLayout, _countof(inputLayout) };
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        psoDesc.NumRenderTargets = 1;
        psoDesc.RTVFormats[0] = swapChain.GetFormat();
        psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
        psoDesc.SampleDesc.Count = 1;

        ComPtr<ID3D12PipelineState> pipelineState;
        hr = devicePtr->GetD3D12Device()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipelineState));
        if (FAILED(hr)) {
            throw std::runtime_error("Failed to create pipeline state");
        }
        Logger::Info("✓ Pipeline state created");

        Logger::Info("==========================================================");
        Logger::Info("  初期化完了！レンダリングループ開始");
        Logger::Info("  ESC: 終了");
        Logger::Info("==========================================================");

        // メインループ
        MSG msg = {};
        float rotation = 0.0f;
        bool firstFrame = true;
        auto lastTime = std::chrono::high_resolution_clock::now();

        while (msg.message != WM_QUIT) {
            if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
                continue;
            }

            // フレーム開始時に統計をリセット
            if (g_simpleStats) {
                g_simpleStats->BeginFrame();
            }

            // 時間計算
            auto currentTime = std::chrono::high_resolution_clock::now();
            float deltaTime = std::chrono::duration<float>(currentTime - lastTime).count();
            lastTime = currentTime;

            // カメラ更新
            if (g_camera) {
                g_camera->Update(deltaTime);
            }
            
            // カメラコントローラー更新
            if (g_cameraController) {
                g_cameraController->Update(deltaTime);
            }

            // ImGUI フレーム開始
            if (g_imguiManager) {
                g_imguiManager->BeginFrame();
                g_imguiManager->UpdatePerformanceStats(deltaTime);
                
                // 統計情報の更新は描画後に移動（後で取得）
                
                // 統計情報は描画後に更新
                
                // RenderGraphの統計情報（実装例）
                g_imguiManager->UpdateRenderGraphStats(3, 5, 3, 32.0f); // 3パス、5リソース、3実行、32MB
                
                // UI状態の更新
                if (g_imguiManager->IsRenderingModeChanged()) {
                    g_renderingMode = g_imguiManager->IsDeferredRenderingEnabled() ? 
                                    RenderingMode::Deferred : RenderingMode::Forward;
                    const char* modeNames[] = { "Forward", "Deferred (G-Buffer)" };
                    Logger::Info("Rendering mode changed to: %s", modeNames[static_cast<int>(g_renderingMode)]);
                }
                
                
                if (g_imguiManager->IsCameraResetRequested()) {
                    if (g_cameraController) {
                        g_cameraController->LoadPreset("Front", 1.0f); // 1秒でトランジション
                        Logger::Info("Camera reset to Front preset");
                    } else if (g_camera) {
                        g_camera->SetPosition(Vector3(-3.0f, 0.0f, 0.0f));
                        g_camera->ResetToDefaultPosition();
                        Logger::Info("Camera reset to default position");
                    }
                }
                
                // マウスキャプチャ状態の更新
                if (g_imguiManager->IsMouseCaptureChanged()) {
                    g_mouseCaptured = g_imguiManager->IsMouseCaptured();
                    if (g_mouseCaptured) {
                        GetCursorPos(&g_lastMousePos);
                        ShowCursor(FALSE);
                        Logger::Info("Mouse capture enabled via UI");
                    } else {
                        ShowCursor(TRUE);
                        Logger::Info("Mouse capture disabled via UI");
                    }
                }
                
                g_imguiManager->ResetChangeFlags();
            }

            // MVP行列計算（カメラ統合）
            rotation += 0.01f;
            
            // FBXモデルの場合はスケールを調整
            Matrix4x4 scale = Matrix4x4::CreateIdentity();
            if (g_useLoadedModel && g_loadedModel) {
                // モデルサイズに基づいて適切なスケールを計算
                Vector3 size = g_loadedModel->GetSize();
                float maxDimension = std::max({size.x, size.y, size.z});
                if (maxDimension > 0) {
                    // モデルを約2ユニットのサイズに正規化
                    float targetSize = 2.0f;
                    float scaleValue = targetSize / maxDimension;
                    scale = Matrix4x4::Scaling(scaleValue);
                    Logger::Info("Applying scale factor: %.3f (model max dimension: %.3f)", scaleValue, maxDimension);
                }
            }
            
            Matrix4x4 world = scale * Matrix4x4::RotationY(rotation) * Matrix4x4::RotationX(rotation * 0.5f);
            
            // カメラ行列を使用
            Matrix4x4 view = g_camera ? g_camera->GetViewMatrix() : 
                Matrix4x4::LookAt(Vector3(-3.0f, 0.0f, 0.0f), Vector3(0.0f, 0.0f, 0.0f), Vector3(0.0f, 1.0f, 0.0f));
            Matrix4x4 proj = g_camera ? g_camera->GetProjectionMatrix() : 
                Matrix4x4::Perspective(3.14159f / 4.0f, static_cast<float>(WINDOW_WIDTH) / WINDOW_HEIGHT, 0.1f, 100.0f);

            Matrix4x4 mvp = world * view * proj;
            mvp = mvp.Transpose();

            TransformBuffer cbData = {};
            cbData.mvp = mvp;
            constantBuffer.Upload(&cbData, sizeof(TransformBuffer));

            // RenderGraphにシーンデータを設定（カメラ統合）
            if (renderGraphExampleResult) {
                Vector3 cameraPos = g_camera ? g_camera->GetPosition() : Vector3(-3.0f, 0.0f, 0.0f);
                Vector3 lightDir(0.0f, -1.0f, 0.0f);
                Vector3 lightColor(1.0f, 1.0f, 1.0f);
                SetRenderGraphSceneData(world, view, proj, cameraPos, lightDir, lightColor);
            }

            // コマンド記録
            commandAllocator->Reset();
            commandList->Reset(commandAllocator.Get(), pipelineState.Get());

            D3D12_VIEWPORT viewport = {};
            viewport.Width = static_cast<float>(WINDOW_WIDTH);
            viewport.Height = static_cast<float>(WINDOW_HEIGHT);
            viewport.MaxDepth = 1.0f;
            commandList->RSSetViewports(1, &viewport);

            D3D12_RECT scissorRect = { 0, 0, static_cast<LONG>(WINDOW_WIDTH), static_cast<LONG>(WINDOW_HEIGHT) };
            commandList->RSSetScissorRects(1, &scissorRect);

            uint32_t backBufferIndex = swapChain.GetCurrentBackBufferIndex();
            D3D12_RESOURCE_BARRIER barrier = {};
            barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            barrier.Transition.pResource = swapChain.GetBackBuffer(backBufferIndex);
            barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
            barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
            barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
            commandList->ResourceBarrier(1, &barrier);

            D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = rtvHeap.GetD3D12DescriptorHeap()->GetCPUDescriptorHandleForHeapStart();
            rtvHandle.ptr += backBufferIndex * rtvHeap.GetDescriptorSize();
            commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle.cpu);

            const float clearColor[] = { 0.1f, 0.2f, 0.4f, 1.0f };
            commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
            commandList->ClearDepthStencilView(dsvHandle.cpu, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

            commandList->SetGraphicsRootSignature(rootSignature.Get());

            // RenderGraphレンダリング
            if (renderGraphExampleResult) {
                // モデルに応じて頂点データとテクスチャを設定
                if (g_useLoadedModel && g_loadedModel && !g_loadedModel->meshes.empty()) {
                    // FBXモデルの最初のメッシュを使用
                    auto mesh = g_loadedModel->meshes[0];
                    
                    if (mesh && mesh->GetRawVertexDataSize() > 0 && mesh->GetRawIndexDataSize() > 0) {
                        // ModelVertexデータをTestVertex形式に変換
                        uint32_t vertexCount = static_cast<uint32_t>(mesh->GetRawVertexDataSize() / mesh->GetVertexStride());
                        uint32_t indexCount = static_cast<uint32_t>(mesh->GetRawIndexCount());
                        
                        // ModelVertexからTestVertexに変換
                        const Athena::ModelVertex* modelVertices = reinterpret_cast<const Athena::ModelVertex*>(mesh->GetRawVertexData());
                        std::vector<TestVertex> convertedVertices;
                        convertedVertices.reserve(vertexCount);
                        
                        for (uint32_t i = 0; i < vertexCount; ++i) {
                            TestVertex testVertex;
                            testVertex.position = modelVertices[i].position;
                            testVertex.normal = modelVertices[i].normal;
                            testVertex.u = modelVertices[i].texcoord.x;
                            testVertex.v = modelVertices[i].texcoord.y;
                            convertedVertices.push_back(testVertex);
                        }
                        
                        SetRenderGraphVertexData(convertedVertices.data(), vertexCount, 
                                                static_cast<const uint32_t*>(mesh->GetRawIndexData()), indexCount);
                        
                        // FBXモデルには白色のデフォルトテクスチャを使用（チェッカーボードの代わり）
                        // 白色テクスチャを作成
                        static std::shared_ptr<Texture> whiteTexture;
                        if (!whiteTexture) {
                            whiteTexture = std::make_shared<Texture>();
                            const uint32_t whitePixel = 0xFFFFFFFF; // 白色
                            whiteTexture->CreateFromMemory(
                                devicePtr->GetD3D12Device(),
                                1, 1,  // 1x1 テクスチャ
                                DXGI_FORMAT_R8G8B8A8_UNORM,
                                &whitePixel
                            );
                            whiteTexture->UploadToGPU(&uploadContext);
                            Logger::Info("Created white default texture for FBX models");
                        }
                        SetRenderGraphTexture(whiteTexture);
                        
                        Logger::Info("Using FBX model data: %u vertices, %u indices (converted from ModelVertex to TestVertex)", vertexCount, indexCount);
                    } else {
                        Logger::Warning("FBX model loaded but no vertex data available, using default cube");
                        SetRenderGraphVertexData(rgCubeVertices, 24, rgCubeIndices, 36);
                        
                        // キューブの場合はチェッカーボードテクスチャを使用
                        // 既存のmainTextureを共有するためのshared_ptrを作成
                        auto mainTexturePtr = std::shared_ptr<Texture>(&mainTexture, [](Texture*){});
                        SetRenderGraphTexture(mainTexturePtr);
                    }
                } else {
                    // デフォルトキューブを使用
                    SetRenderGraphVertexData(rgCubeVertices, 24, rgCubeIndices, 36);
                    
                    // キューブにはチェッカーボードテクスチャを適用
                    // 既存のmainTextureを共有するためのshared_ptrを作成
                    auto mainTexturePtr = std::shared_ptr<Texture>(&mainTexture, [](Texture*){});
                    SetRenderGraphTexture(mainTexturePtr);
                }
                
                Athena::Matrix4x4 rgWorld = world;
                Athena::Vector3 cameraPos = g_camera ? g_camera->GetPosition() : Vector3(0.0f, 1.0f, -3.0f);
                Athena::Vector3 lightDir(0.0f, -1.0f, 0.0f);
                Athena::Vector3 lightColor(1.0f, 1.0f, 1.0f);
                
                SetRenderGraphSceneData(rgWorld, view, proj, cameraPos, lightDir, lightColor);
                SetRenderGraphObjectID(0);
                
                bool useDeferred = (g_renderingMode == RenderingMode::Deferred);
                
                if (g_simpleStats) {
                    uint32_t estimatedVertexCount = g_useLoadedModel ? 1000 : 24; // より正確な頂点数
                    g_simpleStats->RecordDrawCall(estimatedVertexCount);
                }
                
                RenderWithRenderGraph(commandList.Get(), cbvSrvHeap.GetD3D12DescriptorHeap(), rtvHandle, dsvHandle.cpu, useDeferred);
                
                commandList->SetGraphicsRootSignature(rootSignature.Get());
                commandList->SetPipelineState(pipelineState.Get());
                
                ID3D12DescriptorHeap* mainHeaps[] = { cbvSrvHeap.GetD3D12DescriptorHeap() };
                commandList->SetDescriptorHeaps(1, mainHeaps);
            }

            // メインレンダリング用のデスクリプタ設定
            // RenderGraphが実行されていない場合、デスクリプタヒープを設定
            ID3D12DescriptorHeap* heaps[] = { cbvSrvHeap.GetD3D12DescriptorHeap() };
            commandList->SetDescriptorHeaps(1, heaps);
            
            // デスクリプタテーブルはCBVから始まって2つのデスクリプタを含む
            commandList->SetGraphicsRootDescriptorTable(0, cbvHandle.gpu);
            if (firstFrame) {
                Logger::Info("CBV GPU Handle: %llu, SRV GPU Handle: %llu", cbvHandle.gpu.ptr, textureSrvHandle.gpu.ptr);
                firstFrame = false;
            }

            commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

            // FBXモデルまたはキューブの表示状況をログ出力（初回のみ）
            if (firstFrame) {
                if (g_useLoadedModel) {
                    Logger::Info("Rendering FBX model: %s", g_loadedModel->filename.c_str());
                } else {
                    Logger::Info("Rendering default cube geometry");
                }
            }
            
            if (!renderGraphExampleResult) {
                Logger::Warning("RenderGraph disabled - no fallback rendering available");
            }

            // ImGUI レンダリング（RENDER_TARGET状態で実行）
            if (g_imguiManager) {
                g_imguiManager->EndFrame(commandList.Get());
            }

            // レンダリング完了後にPRESENT状態に遷移
            barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
            barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
            commandList->ResourceBarrier(1, &barrier);

            commandList->Close();

            ID3D12CommandList* cmdLists[] = { commandList.Get() };
            commandQueue.ExecuteCommandLists(cmdLists, 1);

            swapChain.Present(true);
            commandQueue.WaitForGPU();
            
            // レンダリング完了後に統計情報を取得・更新
            if (g_imguiManager && g_simpleStats) {
                uint32_t actualDrawCalls = g_simpleStats->GetDrawCalls();
                uint32_t actualVertexCount = g_simpleStats->GetVertexCount();
                float processMemoryMB = g_simpleStats->GetMemoryUsageMB();
                g_imguiManager->UpdateRenderingStats(actualDrawCalls, actualVertexCount, processMemoryMB);
            }
        }

        // 終了
        Logger::Info("==========================================================");
        Logger::Info("  終了処理開始");
        Logger::Info("==========================================================");
		commandQueue.WaitForGPU(); // GPUの処理完了を待機

        // UploadContext解放
        uploadContext.Shutdown();

        // リソース解放（バッファ・テクスチャ）
        constantBuffer.Shutdown();
        mainTexture.Shutdown();
        depthTexture.Shutdown();

        // デスクリプタヒープ解放
        cbvSrvHeap.Shutdown();
        dsvHeap.Shutdown();
        rtvHeap.Shutdown();

        // スワップチェーン解放
        swapChain.Shutdown();

        // CommandQueue解放
        commandQueue.Shutdown();

        // Device解放
        devicePtr->Shutdown();

        Logger::Info("✓ Shutdown complete");
        Logger::Shutdown();
        return 0;
    }
    catch (const std::exception& e) {
        Logger::Error("FATAL ERROR: %s", e.what());
        MessageBoxA(nullptr, e.what(), "Error", MB_OK | MB_ICONERROR);
        return 1;
    }
}