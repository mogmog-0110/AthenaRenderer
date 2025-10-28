#include "Athena/Core/Device.h"
#include "Athena/Utils/Logger.h"
#include <stdexcept>

namespace Athena {

    Device::Device() = default;  // デフォルトコンストラクタ

    Device::~Device() = default;  // デフォルトデストラクタ

    void Device::Initialize(bool enableDebugLayer) {
        Logger::Info("Initializing Device...");

        // デバッグレイヤーの有効化（開発中のみ推奨）
        if (enableDebugLayer) {
            EnableDebugLayer();
        }

        // DXGIファクトリ作成
        // DXGI = DirectX Graphics Infrastructure
        // GPUの列挙やSwapChainの作成などを担当
        UINT flags = 0;
        if (enableDebugLayer) {
            flags = DXGI_CREATE_FACTORY_DEBUG;
        }

        HRESULT hr = CreateDXGIFactory2(flags, IID_PPV_ARGS(&factory));
        if (FAILED(hr)) {
            throw std::runtime_error("Failed to create DXGI factory");
        }

        // 最適なアダプター（GPU）を選択
        SelectBestAdapter();

        // D3D12デバイスの作成
        // D3D_FEATURE_LEVEL_12_0 = DirectX 12対応を要求
        hr = D3D12CreateDevice(
            adapter.Get(),              // 使用するGPU
            D3D_FEATURE_LEVEL_12_0,    // 要求する機能レベル
            IID_PPV_ARGS(&device)      // 作成されたデバイスを格納
        );

        if (FAILED(hr)) {
            throw std::runtime_error("Failed to create D3D12 device");
        }

        // 機能サポートのチェック
        CheckFeatureSupport();

        Logger::Info("Device initialized successfully");
    }

    void Device::Shutdown() {
        Logger::Info("Shutting down Device...");

        // ComPtrは自動的にReleaseを呼ぶが、
        // 明示的にリセットすることで順序を制御
        device.Reset();
        adapter.Reset();
        factory.Reset();
    }

    void Device::EnableDebugLayer() {
#ifdef _DEBUG  // デバッグビルドの時のみ有効
        ComPtr<ID3D12Debug> debugController;

        // デバッグインターフェースを取得
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
            // デバッグレイヤーを有効化
            // これにより、DirectX 12のエラーや警告が詳細に表示
            debugController->EnableDebugLayer();
            Logger::Info("D3D12 Debug Layer enabled");
        }
#endif
    }

    void Device::SelectBestAdapter() {
        ComPtr<IDXGIAdapter1> tempAdapter;
        SIZE_T maxVideoMemory = 0;  // 最大VRAM容量

        // すべてのアダプター（GPU）を列挙
        for (UINT i = 0;
            factory->EnumAdapters1(i, &tempAdapter) != DXGI_ERROR_NOT_FOUND;
            ++i) {

            DXGI_ADAPTER_DESC1 desc;
            tempAdapter->GetDesc1(&desc);

            // ソフトウェアアダプター（仮想GPU）はスキップ
            if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
                continue;
            }

            // DirectX 12対応をチェック
            if (SUCCEEDED(D3D12CreateDevice(
                tempAdapter.Get(),
                D3D_FEATURE_LEVEL_12_0,
                __uuidof(ID3D12Device),
                nullptr))) {  // nullptr = 実際には作成しない（チェックのみ）

                // 最大VRAMを持つGPUを選択
                if (desc.DedicatedVideoMemory > maxVideoMemory) {
                    maxVideoMemory = desc.DedicatedVideoMemory;
                    tempAdapter.As(&adapter);  // ComPtrのキャスト
                }
            }
        }

        if (!adapter) {
            throw std::runtime_error("No compatible adapter found");
        }

        // 選択されたGPU情報をログ出力
        DXGI_ADAPTER_DESC1 desc;
        adapter->GetDesc1(&desc);

        Logger::Info("Selected GPU: %S", desc.Description);  // %S = ワイド文字列
        Logger::Info("VRAM: %.2f GB",
            desc.DedicatedVideoMemory / (1024.0 * 1024.0 * 1024.0));
    }

    void Device::CheckFeatureSupport() {
        // Mesh Shader対応のチェック
        D3D12_FEATURE_DATA_D3D12_OPTIONS7 options7 = {};
        if (SUCCEEDED(device->CheckFeatureSupport(
            D3D12_FEATURE_D3D12_OPTIONS7,
            &options7, sizeof(options7)))) {
            supportMeshShaders = (options7.MeshShaderTier >= D3D12_MESH_SHADER_TIER_1);
        }

        // Raytracing対応のチェック
        D3D12_FEATURE_DATA_D3D12_OPTIONS5 options5 = {};
        if (SUCCEEDED(device->CheckFeatureSupport(
            D3D12_FEATURE_D3D12_OPTIONS5,
            &options5, sizeof(options5)))) {
            supportRaytracing = (options5.RaytracingTier >= D3D12_RAYTRACING_TIER_1_0);
        }

        // Variable Rate Shading対応のチェック
        D3D12_FEATURE_DATA_D3D12_OPTIONS6 options6 = {};
        if (SUCCEEDED(device->CheckFeatureSupport(
            D3D12_FEATURE_D3D12_OPTIONS6,
            &options6, sizeof(options6)))) {
            supportVRS = (options6.VariableShadingRateTier >=
                D3D12_VARIABLE_SHADING_RATE_TIER_1);
        }

        // サポート状況をログ出力
        Logger::Info("Feature Support:");
        Logger::Info("  Mesh Shaders: %s", supportMeshShaders ? "Yes" : "No");
        Logger::Info("  Raytracing: %s", supportRaytracing ? "Yes" : "No");
        Logger::Info("  VRS: %s", supportVRS ? "Yes" : "No");
    }

} // namespace Athena