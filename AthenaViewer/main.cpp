#include <Windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <wrl/client.h>
#include <exception>
#include <cmath>
#include <stdexcept>

#include "Athena/Utils/Logger.h"
#include "Athena/Core/Device.h"
#include "Athena/Core/CommandQueue.h"
#include "Athena/Core/SwapChain.h"
#include "Athena/Core/DescriptorHeap.h"
#include "Athena/Resources/Buffer.h"
#include "Athena/Utils/Math.h"

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

using Microsoft::WRL::ComPtr;

// 頂点構造体（位置 + 色）
struct Vertex {
    Athena::Vector3 position;
    Athena::Vector3 color;
};

// 定数バッファ構造体（MVP行列）
struct ConstantBuffer {
    Athena::Matrix4x4 mvp;
};

// グローバル変数
const int WINDOW_WIDTH = 1280;
const int WINDOW_HEIGHT = 720;
bool g_running = true;
float g_rotation = 0.0f;

// ウィンドウプロシージャ
LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_DESTROY:
        g_running = false;
        PostQuitMessage(0);
        return 0;
    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE) {
            g_running = false;
            PostQuitMessage(0);
        }
        return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

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
        Athena::Logger::Initialize();
        Athena::Logger::Info("=== Athena Renderer Phase 2: Rotating Cube ===");

        // ========================================
        // ウィンドウ作成
        // ========================================
        WNDCLASSEX wc = {};
        wc.cbSize = sizeof(WNDCLASSEX);
        wc.style = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc = WindowProc;
        wc.hInstance = hInstance;
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        wc.lpszClassName = L"AthenaRendererClass";
        RegisterClassEx(&wc);

        RECT windowRect = { 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT };
        AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

        HWND hwnd = CreateWindowEx(
            0,
            wc.lpszClassName,
            L"Athena Renderer - Phase 2: Rotating Cube",
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT, CW_USEDEFAULT,
            windowRect.right - windowRect.left,
            windowRect.bottom - windowRect.top,
            NULL, NULL, hInstance, NULL
        );

        if (!hwnd) {
            throw std::runtime_error("Failed to create window");
        }

        ShowWindow(hwnd, nCmdShow);
        UpdateWindow(hwnd);

        // ========================================
        // DirectX 12 初期化
        // ========================================
        Athena::Device device;
        device.Initialize(true);

        Athena::CommandQueue commandQueue;
        commandQueue.Initialize(device.GetD3D12Device(), D3D12_COMMAND_LIST_TYPE_DIRECT);

        Athena::SwapChain swapChain;
        swapChain.Initialize(
            device.GetDXGIFactory(),
            commandQueue.GetD3D12CommandQueue(),
            hwnd,
            WINDOW_WIDTH, WINDOW_HEIGHT
        );

        // RTV用デスクリプタヒープ
        Athena::DescriptorHeap rtvHeap;
        rtvHeap.Initialize(device.GetD3D12Device(), D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 3);

        // DSV用デスクリプタヒープ（深度バッファ用）
        Athena::DescriptorHeap dsvHeap;
        dsvHeap.Initialize(device.GetD3D12Device(), D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1);

        // RTVの作成
        std::vector<Athena::DescriptorHandle> rtvHandles;
        for (uint32_t i = 0; i < 3; ++i) {
            auto handle = rtvHeap.Allocate();
            device.GetD3D12Device()->CreateRenderTargetView(
                swapChain.GetBackBuffer(i),
                nullptr,
                handle.cpu
            );
            rtvHandles.push_back(handle);
        }

        // ========================================
        // 深度バッファの作成
        // ========================================
        D3D12_RESOURCE_DESC depthDesc = {};
        depthDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        depthDesc.Width = WINDOW_WIDTH;
        depthDesc.Height = WINDOW_HEIGHT;
        depthDesc.DepthOrArraySize = 1;
        depthDesc.MipLevels = 1;
        depthDesc.Format = DXGI_FORMAT_D32_FLOAT;
        depthDesc.SampleDesc.Count = 1;
        depthDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

        D3D12_CLEAR_VALUE clearValue = {};
        clearValue.Format = DXGI_FORMAT_D32_FLOAT;
        clearValue.DepthStencil.Depth = 1.0f;

        D3D12_HEAP_PROPERTIES heapProps = {};
        heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

        ComPtr<ID3D12Resource> depthBuffer;
        device.GetD3D12Device()->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &depthDesc,
            D3D12_RESOURCE_STATE_DEPTH_WRITE,
            &clearValue,
            IID_PPV_ARGS(&depthBuffer)
        );

        auto dsvHandle = dsvHeap.Allocate();
        device.GetD3D12Device()->CreateDepthStencilView(
            depthBuffer.Get(),
            nullptr,
            dsvHandle.cpu
        );

        // ========================================
        // キューブのジオメトリ
        // ========================================
        // 8頂点（立方体の角）
        Vertex vertices[] = {
            // 前面
            { {-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f} }, // 0: 左下前
            { {-0.5f,  0.5f, -0.5f}, {0.0f, 1.0f, 0.0f} }, // 1: 左上前
            { { 0.5f,  0.5f, -0.5f}, {0.0f, 0.0f, 1.0f} }, // 2: 右上前
            { { 0.5f, -0.5f, -0.5f}, {1.0f, 1.0f, 0.0f} }, // 3: 右下前
            // 背面
            { {-0.5f, -0.5f,  0.5f}, {1.0f, 0.0f, 1.0f} }, // 4: 左下後
            { {-0.5f,  0.5f,  0.5f}, {0.0f, 1.0f, 1.0f} }, // 5: 左上後
            { { 0.5f,  0.5f,  0.5f}, {1.0f, 1.0f, 1.0f} }, // 6: 右上後
            { { 0.5f, -0.5f,  0.5f}, {0.5f, 0.5f, 0.5f} }, // 7: 右下後
        };

        // 36インデックス（12三角形 × 3頂点）
        uint32_t indices[] = {
            // 前面
            0, 1, 2,  0, 2, 3,
            // 背面
            4, 6, 5,  4, 7, 6,
            // 左面
            4, 5, 1,  4, 1, 0,
            // 右面
            3, 2, 6,  3, 6, 7,
            // 上面
            1, 5, 6,  1, 6, 2,
            // 下面
            4, 0, 3,  4, 3, 7
        };

        // 頂点バッファ
        Athena::Buffer vertexBuffer;
        vertexBuffer.Initialize(
            device.GetD3D12Device(),
            sizeof(vertices),
            Athena::BufferType::Vertex,
            D3D12_HEAP_TYPE_UPLOAD
        );
        vertexBuffer.Upload(vertices, sizeof(vertices));

        // インデックスバッファ
        Athena::Buffer indexBuffer;
        indexBuffer.Initialize(
            device.GetD3D12Device(),
            sizeof(indices),
            Athena::BufferType::Index,
            D3D12_HEAP_TYPE_UPLOAD
        );
        indexBuffer.Upload(indices, sizeof(indices));

        // 定数バッファ（トリプルバッファリング用に3つ）
        std::vector<Athena::Buffer> constantBuffers(3);
        for (int i = 0; i < 3; ++i) {
            constantBuffers[i].Initialize(
                device.GetD3D12Device(),
                256, // 定数バッファは256バイトアライメント
                Athena::BufferType::Constant,
                D3D12_HEAP_TYPE_UPLOAD
            );
        }

        // ========================================
        // シェーダーコンパイル
        // ========================================
        ComPtr<ID3DBlob> vertexShader;
        ComPtr<ID3DBlob> pixelShader;
        ComPtr<ID3DBlob> errorBlob;

        UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;

        // 頂点シェーダー
        HRESULT hr = D3DCompileFromFile(
            L"../shaders/CubeVS.hlsl",
            nullptr, nullptr,
            "main", "vs_5_0",
            compileFlags, 0,
            &vertexShader, &errorBlob
        );
        if (FAILED(hr)) {
            if (errorBlob) {
                Athena::Logger::Error("Vertex Shader compile error: %s",
                    (char*)errorBlob->GetBufferPointer());
            }
            throw std::runtime_error("Failed to compile vertex shader");
        }

        // ピクセルシェーダー
        hr = D3DCompileFromFile(
            L"../shaders/CubePS.hlsl",
            nullptr, nullptr,
            "main", "ps_5_0",
            compileFlags, 0,
            &pixelShader, &errorBlob
        );
        if (FAILED(hr)) {
            if (errorBlob) {
                Athena::Logger::Error("Pixel Shader compile error: %s",
                    (char*)errorBlob->GetBufferPointer());
            }
            throw std::runtime_error("Failed to compile pixel shader");
        }

        // ========================================
        // ルートシグネチャ
        // ========================================
        D3D12_ROOT_PARAMETER rootParam = {};
        rootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
        rootParam.Descriptor.ShaderRegister = 0; // b0
        rootParam.Descriptor.RegisterSpace = 0;
        rootParam.ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

        D3D12_ROOT_SIGNATURE_DESC rootSigDesc = {};
        rootSigDesc.NumParameters = 1;
        rootSigDesc.pParameters = &rootParam;
        rootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

        ComPtr<ID3DBlob> signature;
        D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &errorBlob);

        ComPtr<ID3D12RootSignature> rootSignature;
        device.GetD3D12Device()->CreateRootSignature(
            0,
            signature->GetBufferPointer(),
            signature->GetBufferSize(),
            IID_PPV_ARGS(&rootSignature)
        );

        // ========================================
        // パイプラインステート
        // ========================================
        D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
        };

        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
        psoDesc.pRootSignature = rootSignature.Get();
        psoDesc.VS = { vertexShader->GetBufferPointer(), vertexShader->GetBufferSize() };
        psoDesc.PS = { pixelShader->GetBufferPointer(), pixelShader->GetBufferSize() };
        psoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
        psoDesc.SampleMask = UINT_MAX;
        psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
        psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
        psoDesc.RasterizerState.DepthClipEnable = TRUE;
        psoDesc.InputLayout = { inputLayout, _countof(inputLayout) };
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        psoDesc.NumRenderTargets = 1;
        psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
        psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
        psoDesc.SampleDesc.Count = 1;

        // 深度ステンシル設定
        psoDesc.DepthStencilState.DepthEnable = TRUE;
        psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
        psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;

        ComPtr<ID3D12PipelineState> pipelineState;
        device.GetD3D12Device()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipelineState));

        // ========================================
        // コマンドアロケータとコマンドリスト
        // ========================================
        std::vector<ComPtr<ID3D12CommandAllocator>> commandAllocators(3);
        for (int i = 0; i < 3; ++i) {
            device.GetD3D12Device()->CreateCommandAllocator(
                D3D12_COMMAND_LIST_TYPE_DIRECT,
                IID_PPV_ARGS(&commandAllocators[i])
            );
        }

        ComPtr<ID3D12GraphicsCommandList> commandList;
        device.GetD3D12Device()->CreateCommandList(
            0,
            D3D12_COMMAND_LIST_TYPE_DIRECT,
            commandAllocators[0].Get(),
            nullptr,
            IID_PPV_ARGS(&commandList)
        );
        commandList->Close();

        // ビューポートとシザー矩形
        D3D12_VIEWPORT viewport = {};
        viewport.Width = static_cast<float>(WINDOW_WIDTH);
        viewport.Height = static_cast<float>(WINDOW_HEIGHT);
        viewport.MinDepth = 0.0f;
        viewport.MaxDepth = 1.0f;

        D3D12_RECT scissorRect = {};
        scissorRect.right = WINDOW_WIDTH;
        scissorRect.bottom = WINDOW_HEIGHT;

        Athena::Logger::Info("Starting render loop...");

        // ========================================
        // メインループ
        // ========================================
        MSG msg = {};
        while (g_running) {
            // メッセージ処理
            while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }

            // 回転角度更新
            g_rotation += 0.01f;

            uint32_t frameIndex = swapChain.GetCurrentBackBufferIndex();

            // コマンドアロケータリセット
            commandAllocators[frameIndex]->Reset();
            commandList->Reset(commandAllocators[frameIndex].Get(), pipelineState.Get());

            // MVP行列の計算
            using namespace Athena;
            Matrix4x4 model = Matrix4x4::RotationY(g_rotation) * Matrix4x4::RotationX(g_rotation * 0.5f);
            Matrix4x4 view = Matrix4x4::LookAtLH(
                Vector3(0, 0, -3),  // カメラ位置
                Vector3(0, 0, 0),   // 注視点
                Vector3(0, 1, 0)    // 上方向
            );
            Matrix4x4 proj = Matrix4x4::PerspectiveFovLH(
                ToRadians(60.0f),   // 視野角
                (float)WINDOW_WIDTH / WINDOW_HEIGHT, // アスペクト比
                0.1f, 100.0f        // Near, Far
            );
            Matrix4x4 mvp = model * view * proj;

            // HLSL用に転置（列優先に変換）
            mvp = mvp.Transpose();

            // 定数バッファ更新
            ConstantBuffer cb = { mvp };
            constantBuffers[frameIndex].Upload(&cb, sizeof(cb));

            // レンダーターゲットへの遷移
            D3D12_RESOURCE_BARRIER barrier = {};
            barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            barrier.Transition.pResource = swapChain.GetCurrentBackBuffer();
            barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
            barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
            commandList->ResourceBarrier(1, &barrier);

            // レンダーターゲット設定
            commandList->OMSetRenderTargets(1, &rtvHandles[frameIndex].cpu, FALSE, &dsvHandle.cpu);

            // 画面クリア
            float clearColor[] = { 0.1f, 0.2f, 0.4f, 1.0f };
            commandList->ClearRenderTargetView(rtvHandles[frameIndex].cpu, clearColor, 0, nullptr);
            commandList->ClearDepthStencilView(dsvHandle.cpu, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

            // 描画設定
            commandList->SetGraphicsRootSignature(rootSignature.Get());
            commandList->SetGraphicsRootConstantBufferView(0, constantBuffers[frameIndex].GetGPUVirtualAddress());
            commandList->RSSetViewports(1, &viewport);
            commandList->RSSetScissorRects(1, &scissorRect);
            commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

            D3D12_VERTEX_BUFFER_VIEW vbv = vertexBuffer.GetVertexBufferView();
            vbv.StrideInBytes = sizeof(Vertex);
            commandList->IASetVertexBuffers(0, 1, &vbv);

            D3D12_INDEX_BUFFER_VIEW ibv = indexBuffer.GetIndexBufferView();
            commandList->IASetIndexBuffer(&ibv);

            // 描画
            commandList->DrawIndexedInstanced(36, 1, 0, 0, 0);

            // PRESENTへの遷移
            barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
            barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
            commandList->ResourceBarrier(1, &barrier);

            commandList->Close();

            // コマンド実行
            ID3D12CommandList* cmdLists[] = { commandList.Get() };
            commandQueue.ExecuteCommandLists(cmdLists, 1);

            // プレゼント
            swapChain.Present(true);

            // GPU待機
            commandQueue.WaitForGPU();
        }

        // ========================================
        // 終了処理
        // ========================================
        Athena::Logger::Info("Shutting down...");

        commandQueue.WaitForGPU();

        for (auto& cb : constantBuffers) {
            cb.Shutdown();
        }
        indexBuffer.Shutdown();
        vertexBuffer.Shutdown();
        dsvHeap.Shutdown();
        rtvHeap.Shutdown();
        swapChain.Shutdown();
        commandQueue.Shutdown();
        device.Shutdown();

        DestroyWindow(hwnd);
        Athena::Logger::Shutdown();

        return 0;
    }
    catch (const std::exception& e) {
        Athena::Logger::Error("Fatal error: %s", e.what());
        MessageBoxA(NULL, e.what(), "Error", MB_OK | MB_ICONERROR);
        return 1;
    }
}