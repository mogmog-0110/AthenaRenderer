#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>

namespace Athena {

    // ComPtr: COMオブジェクト用のスマートポインタ
    // 自動的にAddRef/Releaseを呼んでくれるので、メモリリークを防げる
    using Microsoft::WRL::ComPtr;

    /**
     * @brief DirectX 12デバイスの管理クラス
     *
     * GPUとの通信を行うための中心的なクラス。
     * - GPUアダプター（物理GPU）の選択
     * - ID3D12Device（DirectX 12の中核オブジェクト）の作成
     * - 機能サポートのチェック（Mesh Shader、Raytracingなど）
     */
    class Device {
    public:
        Device();
        ~Device();

        /**
         * @brief デバイスを初期化
         * @param enableDebugLayer デバッグレイヤーを有効にするか
         *                         （開発中はtrue、リリース時はfalse）
         */
        void Initialize(bool enableDebugLayer = true);

        /**
         * @brief デバイスをシャットダウン
         */
        void Shutdown();

        // アクセサ（ゲッター関数）
        // これらを通じて他のクラスがDirectX 12のオブジェクトにアクセス
        ID3D12Device5* GetD3D12Device() const { return device.Get(); }
        IDXGIFactory6* GetDXGIFactory() const { return factory.Get(); }
        IDXGIAdapter4* GetAdapter() const { return adapter.Get(); }

        // 機能サポート確認
        // GPUがMesh ShaderやRaytracingに対応しているかを確認
        bool SupportsMeshShaders() const { return supportMeshShaders; }
        bool SupportsRaytracing() const { return supportRaytracing; }
        bool SupportsVariableRateShading() const { return supportVRS; }

    private:
        // ComPtr<T>: COMオブジェクトを自動管理するスマートポインタ
        ComPtr<ID3D12Device5> device;        // DirectX 12デバイス本体
        ComPtr<IDXGIFactory6> factory;       // デバイス作成やSwapChain作成用
        ComPtr<IDXGIAdapter4> adapter;       // 物理GPU（グラフィックカード）

        // 機能サポートフラグ
        bool supportMeshShaders = false;
        bool supportRaytracing = false;
        bool supportVRS = false;  // VRS = Variable Rate Shading

        // 内部ヘルパー関数
        void EnableDebugLayer();           // デバッグレイヤーの有効化
        void SelectBestAdapter();          // 最適なGPUを選択
        void CheckFeatureSupport();        // 機能サポートをチェック
    };

} // namespace Athena