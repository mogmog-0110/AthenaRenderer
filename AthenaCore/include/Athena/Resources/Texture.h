#pragma once

#include <d3d12.h>
#include <wrl/client.h>
#include <cstdint>
#include <string>
#include <DirectXTex.h>

namespace Athena {

    using Microsoft::WRL::ComPtr;

    // 前方宣言
    class UploadContext;

    /**
     * @brief テクスチャの種類
     */
    enum class TextureType {
        Texture2D,      // 2Dテクスチャ
        TextureCube,    // キューブマップ
        Texture3D,      // 3Dテクスチャ
        RenderTarget,   // レンダーターゲット
        DepthStencil    // 深度ステンシル
    };

    /**
     * @brief テクスチャ抽象化クラス
     *
     * DirectXTexを使用して様々な形式の画像ファイルを読み込み、
     * DirectX 12のテクスチャリソースとして管理する
     */
    class Texture {
    public:
        Texture();
        ~Texture();

        /**
         * @brief 画像ファイルからテクスチャを作成
         * @param device DirectX 12デバイス
         * @param filepath 画像ファイルパス（PNG, JPG, TGA, BMP, DDS, HDR等）
         *
         * DirectXTexを使用して様々な形式に対応：
         * - WIC: PNG, JPG, BMP, GIF, TIFF等
         * - DDS: DirectDraw Surface
         * - TGA: Targa
         * - HDR: High Dynamic Range
         */
        void LoadFromFile(ID3D12Device* device, const std::string& filepath);

        /**
         * @brief GPUにテクスチャをアップロード
         * @param uploadContext アップロードコンテキスト
         *
         * LoadFromFile()の後に呼び出して、実際にGPUにデータを転送する
         */
        void UploadToGPU(UploadContext* uploadContext);

        /**
         * @brief メモリからテクスチャを作成
         * @param device DirectX 12デバイス
         * @param width テクスチャ幅
         * @param height テクスチャ高さ
         * @param format ピクセルフォーマット
         * @param data 画像データ
         */
        void CreateFromMemory(
            ID3D12Device* device,
            uint32_t width,
            uint32_t height,
            DXGI_FORMAT format,
            const void* data
        );

        /**
         * @brief レンダーターゲット用テクスチャを作成
         */
        void CreateRenderTarget(
            ID3D12Device* device,
            uint32_t width,
            uint32_t height,
            DXGI_FORMAT format
        );

        /**
         * @brief 深度ステンシル用テクスチャを作成
         */
        void CreateDepthStencil(
            ID3D12Device* device,
            uint32_t width,
            uint32_t height
        );

        /**
         * @brief テクスチャの終了処理
         */
        void Shutdown();

        /**
         * @brief Shader Resource Viewを作成
         * @param device DirectX 12デバイス
         * @param cpuHandle SRVを作成するデスクリプタハンドル
         */
        void CreateSRV(ID3D12Device* device, D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle);

        /**
         * @brief Render Target Viewを作成
         * @param device DirectX 12デバイス
         * @param cpuHandle RTVを作成するデスクリプタハンドル
         */
        void CreateRTV(ID3D12Device* device, D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle);

        /**
         * @brief Depth Stencil Viewを作成
         * @param device DirectX 12デバイス
         * @param cpuHandle DSVを作成するデスクリプタハンドル
         */
        void CreateDSV(ID3D12Device* device, D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle);

        // アクセサ
        ID3D12Resource* GetResource() const { return resource.Get(); }
        uint32_t GetWidth() const { return width; }
        uint32_t GetHeight() const { return height; }
        DXGI_FORMAT GetFormat() const { return format; }
        TextureType GetType() const { return type; }

    private:
        ComPtr<ID3D12Resource> resource;
        ComPtr<ID3D12Resource> uploadBuffer; // アップロード用一時バッファ

        uint32_t width = 0;
        uint32_t height = 0;
        DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
        TextureType type = TextureType::Texture2D;

        // DirectXTexの一時画像データ（アップロード前）
        DirectX::ScratchImage tempImageData;

        void CreateResource(
            ID3D12Device* device,
            uint32_t width,
            uint32_t height,
            DXGI_FORMAT format,
            D3D12_RESOURCE_FLAGS flags,
            D3D12_RESOURCE_STATES initialState,
            const D3D12_CLEAR_VALUE* clearValue = nullptr
        );
    };

} // namespace Athena