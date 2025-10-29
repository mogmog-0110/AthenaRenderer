#define NOMINMAX

#include <Windows.h>
#include <exception>
#include <stdexcept>
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

using namespace Athena;
using Microsoft::WRL::ComPtr;

// グローバル変数
HWND g_hwnd = nullptr;
const uint32_t WINDOW_WIDTH = 1280;
const uint32_t WINDOW_HEIGHT = 720;

// 頂点構造体
struct Vertex {
    Vector3 position;
    float u, v;
};

// 定数バッファ構造体（256バイトアライメント）
struct TransformBuffer {
    Matrix4x4 mvp;
    float padding[48];
};

// ウィンドウプロシージャ
LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE) PostQuitMessage(0);
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
        L"Athena Renderer - Phase 4: Image Loading",
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
        Logger::Info("  Athena Renderer - Phase 4: Image File Loading");
        Logger::Info("==========================================================");

        // ウィンドウ作成
        if (!CreateAppWindow(hInstance)) {
            throw std::runtime_error("Failed to create window");
        }
        Logger::Info("✓ Window created");

        // デバイス初期化
        Device device;
        device.Initialize(true);
        Logger::Info("✓ Device initialized");

        // コマンドキュー
        CommandQueue commandQueue;
        commandQueue.Initialize(device.GetD3D12Device(), D3D12_COMMAND_LIST_TYPE_DIRECT);
        Logger::Info("✓ CommandQueue initialized");

        // スワップチェーン
        SwapChain swapChain;
        swapChain.Initialize(
            device.GetDXGIFactory(),
            commandQueue.GetD3D12CommandQueue(),
            g_hwnd,
            WINDOW_WIDTH,
            WINDOW_HEIGHT
        );
        Logger::Info("✓ SwapChain initialized");

        // デスクリプタヒープ
        DescriptorHeap rtvHeap;
        rtvHeap.Initialize(device.GetD3D12Device(), D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 3, false);

        DescriptorHeap dsvHeap;
        dsvHeap.Initialize(device.GetD3D12Device(), D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1, false);

        DescriptorHeap cbvSrvHeap;
        cbvSrvHeap.Initialize(device.GetD3D12Device(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 10, true);
        Logger::Info("✓ Descriptor heaps created");

        // RTVを作成
        for (uint32_t i = 0; i < SwapChain::BufferCount; ++i) {
            auto handle = rtvHeap.Allocate();
            D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
            rtvDesc.Format = swapChain.GetFormat();
            rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
            device.GetD3D12Device()->CreateRenderTargetView(
                swapChain.GetBackBuffer(i),
                &rtvDesc,
                handle.cpu
            );
        }
        Logger::Info("✓ RTVs created");

        // 深度バッファ
        Texture depthTexture;
        depthTexture.CreateDepthStencil(device.GetD3D12Device(), WINDOW_WIDTH, WINDOW_HEIGHT);
        auto dsvHandle = dsvHeap.Allocate();
        depthTexture.CreateDSV(device.GetD3D12Device(), dsvHandle.cpu);
        Logger::Info("✓ Depth buffer created");

        // コマンドアロケータ
        ComPtr<ID3D12CommandAllocator> commandAllocator;
        device.GetD3D12Device()->CreateCommandAllocator(
            D3D12_COMMAND_LIST_TYPE_DIRECT,
            IID_PPV_ARGS(&commandAllocator)
        );

        ComPtr<ID3D12GraphicsCommandList> commandList;
        device.GetD3D12Device()->CreateCommandList(
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
        uploadContext.Initialize(device.GetD3D12Device(), commandQueue.GetD3D12CommandQueue());

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
                device.GetD3D12Device(),
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
                device.GetD3D12Device(),
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

        // 頂点データ
        Vertex vertices[] = {
            // 前面
            {{-0.5f, -0.5f,  0.5f}, 0.0f, 1.0f},
            {{-0.5f,  0.5f,  0.5f}, 0.0f, 0.0f},
            {{ 0.5f,  0.5f,  0.5f}, 1.0f, 0.0f},
            {{ 0.5f, -0.5f,  0.5f}, 1.0f, 1.0f},
            // 背面
            {{ 0.5f, -0.5f, -0.5f}, 0.0f, 1.0f},
            {{ 0.5f,  0.5f, -0.5f}, 0.0f, 0.0f},
            {{-0.5f,  0.5f, -0.5f}, 1.0f, 0.0f},
            {{-0.5f, -0.5f, -0.5f}, 1.0f, 1.0f},
            // 上面
            {{-0.5f,  0.5f,  0.5f}, 0.0f, 1.0f},
            {{-0.5f,  0.5f, -0.5f}, 0.0f, 0.0f},
            {{ 0.5f,  0.5f, -0.5f}, 1.0f, 0.0f},
            {{ 0.5f,  0.5f,  0.5f}, 1.0f, 1.0f},
            // 底面
            {{-0.5f, -0.5f, -0.5f}, 0.0f, 1.0f},
            {{-0.5f, -0.5f,  0.5f}, 0.0f, 0.0f},
            {{ 0.5f, -0.5f,  0.5f}, 1.0f, 0.0f},
            {{ 0.5f, -0.5f, -0.5f}, 1.0f, 1.0f},
            // 右面
            {{ 0.5f, -0.5f,  0.5f}, 0.0f, 1.0f},
            {{ 0.5f,  0.5f,  0.5f}, 0.0f, 0.0f},
            {{ 0.5f,  0.5f, -0.5f}, 1.0f, 0.0f},
            {{ 0.5f, -0.5f, -0.5f}, 1.0f, 1.0f},
            // 左面
            {{-0.5f, -0.5f, -0.5f}, 0.0f, 1.0f},
            {{-0.5f,  0.5f, -0.5f}, 0.0f, 0.0f},
            {{-0.5f,  0.5f,  0.5f}, 1.0f, 0.0f},
            {{-0.5f, -0.5f,  0.5f}, 1.0f, 1.0f},
        };

        uint32_t indices[] = {
            0, 1, 2, 0, 2, 3,       // 前面
            4, 5, 6, 4, 6, 7,       // 背面
            8, 9, 10, 8, 10, 11,    // 上面
            12, 13, 14, 12, 14, 15, // 底面
            16, 17, 18, 16, 18, 19, // 右面
            20, 21, 22, 20, 22, 23, // 左面
        };

        // バッファ作成
        Buffer vertexBuffer;
        vertexBuffer.Initialize(
            device.GetD3D12Device(),
            sizeof(vertices),
            BufferType::Vertex,
            D3D12_HEAP_TYPE_UPLOAD
        );
        vertexBuffer.Upload(vertices, sizeof(vertices));

        Buffer indexBuffer;
        indexBuffer.Initialize(
            device.GetD3D12Device(),
            sizeof(indices),
            BufferType::Index,
            D3D12_HEAP_TYPE_UPLOAD
        );
        indexBuffer.Upload(indices, sizeof(indices));

        Buffer constantBuffer;
        constantBuffer.Initialize(
            device.GetD3D12Device(),
            256,
            BufferType::Constant,
            D3D12_HEAP_TYPE_UPLOAD
        );
        Logger::Info("✓ Buffers created");

        // CBV作成
        D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
        cbvDesc.BufferLocation = constantBuffer.GetGPUVirtualAddress();
        cbvDesc.SizeInBytes = 256;
        device.GetD3D12Device()->CreateConstantBufferView(&cbvDesc, cbvHandle.cpu);
        Logger::Info("✓ CBV created");

        // SRV作成
        mainTexture.CreateSRV(device.GetD3D12Device(), textureSrvHandle.cpu);
        Logger::Info("✓ Texture SRV created");

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
        device.GetD3D12Device()->CreateRootSignature(
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
        hr = device.GetD3D12Device()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipelineState));
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


        while (msg.message != WM_QUIT) {
            if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
                continue;
            }

            // MVP行列計算
            rotation += 0.01f;

            Matrix4x4 world = Matrix4x4::RotationY(rotation) * Matrix4x4::RotationX(rotation * 0.5f);
            Matrix4x4 view = Matrix4x4::LookAt(
                Vector3(0.0f, 1.0f, -3.0f),
                Vector3(0.0f, 0.0f, 0.0f),
                Vector3(0.0f, 1.0f, 0.0f)
            );
            Matrix4x4 proj = Matrix4x4::Perspective(
                3.14159f / 4.0f,
                static_cast<float>(WINDOW_WIDTH) / WINDOW_HEIGHT,
                0.1f,
                100.0f
            );

           /* Matrix4x4 mvp = proj * view * world;
            mvp = mvp.Transpose();*/

            Matrix4x4 mvp = world * view * proj;
			mvp = mvp.Transpose();

            TransformBuffer cbData = {};
            cbData.mvp = mvp;
            constantBuffer.Upload(&cbData, sizeof(TransformBuffer));

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

            ID3D12DescriptorHeap* heaps[] = { cbvSrvHeap.GetD3D12DescriptorHeap() };
            commandList->SetDescriptorHeaps(1, heaps);
            commandList->SetGraphicsRootDescriptorTable(0, cbvHandle.gpu);

            commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

            D3D12_VERTEX_BUFFER_VIEW vbv = vertexBuffer.GetVertexBufferView();
            vbv.StrideInBytes = sizeof(Vertex);

            D3D12_INDEX_BUFFER_VIEW ibv = indexBuffer.GetIndexBufferView();

            commandList->IASetVertexBuffers(0, 1, &vbv);
            commandList->IASetIndexBuffer(&ibv);

            commandList->DrawIndexedInstanced(36, 1, 0, 0, 0);

            barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
            barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
            commandList->ResourceBarrier(1, &barrier);

            commandList->Close();

            ID3D12CommandList* cmdLists[] = { commandList.Get() };
            commandQueue.ExecuteCommandLists(cmdLists, 1);

            swapChain.Present(true);
            commandQueue.WaitForGPU();
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
        indexBuffer.Shutdown();
        vertexBuffer.Shutdown();
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
        device.Shutdown();

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