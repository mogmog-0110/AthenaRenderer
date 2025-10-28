#pragma once

#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
#include <cstdint>
#include <vector>

namespace Athena {

    using Microsoft::WRL::ComPtr;

    /**
     * @brief スワップチェーン管理クラス
     *
     * 画面への表示を管理。
     * スワップチェーンは複数のバックバッファを持ち、GPUで描画している間に
     * 別のバッファを画面に表示することで、滑らかな描画を実現する。
     */
    class SwapChain {
    public:
        static constexpr uint32_t BufferCount = 3; // トリプルバッファリング

        SwapChain();
        ~SwapChain();

        /**
         * @brief スワップチェーンを初期化
         *
         * @param factory DXGIファクトリー
         * @param commandQueue コマンドキュー（Present用）
         * @param hwnd ウィンドウハンドル
         * @param width 画面幅
         * @param height 画面高さ
         *
         * トリプルバッファリング（3枚のバックバッファ）を使用：
         * - Buffer 0: 現在画面に表示中
         * - Buffer 1: 次に表示される（準備完了）
         * - Buffer 2: GPUが描画中
         */
        void Initialize(
            IDXGIFactory6* factory,
            ID3D12CommandQueue* commandQueue,
            HWND hwnd,
            uint32_t width,
            uint32_t height
        );

        /**
         * @brief スワップチェーンをシャットダウン
         */
        void Shutdown();

        /**
         * @brief 画面に表示（Present）
         *
         * @param vsync 垂直同期を有効にするか
         *
         * vsync = true:  画面のリフレッシュレートに同期（60Hz なら 60FPS）
         * vsync = false: 可能な限り高速に描画（ティアリングが発生する可能性）
         */
        void Present(bool vsync = true);

        /**
         * @brief 現在のバックバッファインデックスを取得
         *
         * @return バックバッファのインデックス（0, 1, 2のいずれか）
         *
         * Present() を呼ぶたびにインデックスが変わる。
         */
        uint32_t GetCurrentBackBufferIndex() const;

        /**
         * @brief 現在のバックバッファを取得
         *
         * @return ID3D12Resource ポインタ
         *
         * このバッファに描画することで、次のフレームで画面に表示。
         */
        ID3D12Resource* GetCurrentBackBuffer() const;

        /**
         * @brief 指定したインデックスのバックバッファを取得
         *
         * @param index バックバッファのインデックス（0, 1, 2）
         * @return ID3D12Resource ポインタ
         */
        ID3D12Resource* GetBackBuffer(uint32_t index) const;

        /**
         * @brief 画面サイズを変更
         *
         * @param width 新しい幅
         * @param height 新しい高さ
         *
         * ウィンドウサイズが変更されたときに呼び出す。
         * バックバッファを一度解放してから再作成。
         */
        void Resize(uint32_t width, uint32_t height);

        /**
         * @brief DXGIスワップチェーンへのアクセス
         */
        IDXGISwapChain4* GetDXGISwapChain() const { return swapChain.Get(); }

        /**
         * @brief 画面幅を取得
         */
        uint32_t GetWidth() const { return width; }

        /**
         * @brief 画面高さを取得
         */
        uint32_t GetHeight() const { return height; }

        /**
         * @brief バックバッファのフォーマットを取得
         */
        DXGI_FORMAT GetFormat() const { return format; }

    private:
        ComPtr<IDXGISwapChain4> swapChain;                      // スワップチェーン本体
        std::vector<ComPtr<ID3D12Resource>> backBuffers;        // バックバッファ配列

        uint32_t width = 0;                                     // 画面幅
        uint32_t height = 0;                                    // 画面高さ
        DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM;       // カラーフォーマット

        void CreateBackBuffers();
    };

} // namespace Athena