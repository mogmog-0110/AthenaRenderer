#include "Athena/Core/SwapChain.h"
#include "Athena/Utils/Logger.h"
#include <stdexcept>

namespace Athena {

    SwapChain::SwapChain() = default;

    SwapChain::~SwapChain() = default;

    void SwapChain::Initialize(
        IDXGIFactory6* factory,
        ID3D12CommandQueue* commandQueue,
        HWND hwnd,
        uint32_t width,
        uint32_t height) {

        this->width = width;
        this->height = height;

        Logger::Info("Initializing SwapChain (%ux%u)...", width, height);

        // スワップチェーンの設定
        DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
        swapChainDesc.Width = width;                               // 幅
        swapChainDesc.Height = height;                             // 高さ
        swapChainDesc.Format = format;                             // ピクセルフォーマット
        swapChainDesc.Stereo = FALSE;                              // ステレオ表示（VR用）
        swapChainDesc.SampleDesc.Count = 1;                        // MSAAサンプル数（1=無効）
        swapChainDesc.SampleDesc.Quality = 0;                      // MSAA品質
        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;  // レンダーターゲット用
        swapChainDesc.BufferCount = BufferCount;                   // バックバッファ数（3枚）
        swapChainDesc.Scaling = DXGI_SCALING_STRETCH;              // スケーリング方法
        swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;  // フリップモデル
        swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;     // アルファ処理
        swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;  // ティアリング許可

        // スワップチェーン作成（まずIDXGISwapChain1として作成）
        ComPtr<IDXGISwapChain1> swapChain1;
        HRESULT hr = factory->CreateSwapChainForHwnd(
            commandQueue,           // コマンドキュー
            hwnd,                   // ウィンドウハンドル
            &swapChainDesc,         // スワップチェーン設定
            nullptr,                // フルスクリーン設定（nullptr=ウィンドウモード）
            nullptr,                // 出力制限（nullptr=制限なし）
            &swapChain1
        );

        if (FAILED(hr)) {
            throw std::runtime_error("Failed to create swap chain");
        }

        // IDXGISwapChain4にキャスト（最新インターフェース）
        swapChain1.As(&swapChain);

        // Alt+Enterでのフルスクリーン切り替えを無効化
        // （手動でフルスクリーン管理したい場合）
        factory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER);

        // バックバッファの取得
        CreateBackBuffers();

        Logger::Info("SwapChain initialized successfully");
    }

    void SwapChain::Shutdown() {
        Logger::Info("Shutting down SwapChain...");

        backBuffers.clear();
        swapChain.Reset();

        Logger::Info("SwapChain shut down successfully");
    }

    void SwapChain::Present(bool vsync) {
        // 垂直同期の設定
        UINT syncInterval = vsync ? 1 : 0;
        UINT flags = vsync ? 0 : DXGI_PRESENT_ALLOW_TEARING;

        // 画面に表示
        swapChain->Present(syncInterval, flags);
    }

    uint32_t SwapChain::GetCurrentBackBufferIndex() const {
        return swapChain->GetCurrentBackBufferIndex();
    }

    ID3D12Resource* SwapChain::GetCurrentBackBuffer() const {
        return backBuffers[GetCurrentBackBufferIndex()].Get();
    }

    ID3D12Resource* SwapChain::GetBackBuffer(uint32_t index) const {
        if (index >= BufferCount) {
            Logger::Error("Invalid back buffer index: %u", index);
            return nullptr;
        }
        return backBuffers[index].Get();
    }

    void SwapChain::Resize(uint32_t width, uint32_t height) {
        // サイズが変わっていなければ何もしない
        if (this->width == width && this->height == height) {
            return;
        }

        Logger::Info("Resizing SwapChain to %ux%u...", width, height);

        this->width = width;
        this->height = height;

        // バックバッファを一度解放
        backBuffers.clear();

        // スワップチェーンのリサイズ
        HRESULT hr = swapChain->ResizeBuffers(
            BufferCount,                            // バッファ数
            width,                                  // 新しい幅
            height,                                 // 新しい高さ
            format,                                 // フォーマット
            DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING     // フラグ
        );

        if (FAILED(hr)) {
            throw std::runtime_error("Failed to resize swap chain");
        }

        // バックバッファを再取得
        CreateBackBuffers();

        Logger::Info("SwapChain resized successfully");
    }

    void SwapChain::CreateBackBuffers() {
        backBuffers.resize(BufferCount);

        // 各バックバッファを取得
        for (uint32_t i = 0; i < BufferCount; ++i) {
            HRESULT hr = swapChain->GetBuffer(i, IID_PPV_ARGS(&backBuffers[i]));
            if (FAILED(hr)) {
                throw std::runtime_error("Failed to get back buffer");
            }

            // デバッグ用の名前を設定（PIXなどで表示される）
            wchar_t name[32];
            swprintf_s(name, L"BackBuffer[%u]", i);
            backBuffers[i]->SetName(name);
        }

        Logger::Info("Created %u back buffers", BufferCount);
    }

} // namespace Athena