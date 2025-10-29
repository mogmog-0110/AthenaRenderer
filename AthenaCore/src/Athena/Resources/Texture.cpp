#include "Athena/Resources/Texture.h"
#include "Athena/Resources/UploadContext.h"
#include "Athena/Utils/Logger.h"
#include <DirectXTex.h>
#include <stdexcept>

namespace Athena {

    Texture::Texture() = default;

    Texture::~Texture() {
        Shutdown();
    }

    void Texture::LoadFromFile(ID3D12Device* device, const std::string& filepath) {
        Logger::Info("Loading texture: %s", filepath.c_str());

        // std::string → std::wstring に変換
        std::wstring wfilepath(filepath.begin(), filepath.end());

        // DirectXTexで画像を読み込み
        DirectX::TexMetadata metadata;
        DirectX::ScratchImage image;
        HRESULT hr;

        // 拡張子を取得
        std::wstring ext = wfilepath.substr(wfilepath.find_last_of(L".") + 1);

        // 拡張子に応じた読み込み
        if (ext == L"dds" || ext == L"DDS") {
            hr = DirectX::LoadFromDDSFile(
                wfilepath.c_str(),
                DirectX::DDS_FLAGS_NONE,
                &metadata,
                image
            );
        }
        else if (ext == L"tga" || ext == L"TGA") {
            hr = DirectX::LoadFromTGAFile(
                wfilepath.c_str(),
                &metadata,
                image
            );
        }
        else if (ext == L"hdr" || ext == L"HDR") {
            hr = DirectX::LoadFromHDRFile(
                wfilepath.c_str(),
                &metadata,
                image
            );
        }
        else {
            // PNG, JPG, BMP等はWIC (Windows Imaging Component) で読み込み
            hr = DirectX::LoadFromWICFile(
                wfilepath.c_str(),
                DirectX::WIC_FLAGS_NONE,
                &metadata,
                image
            );
        }

        if (FAILED(hr)) {
            Logger::Error("Failed to load texture: %s (HRESULT: 0x%08X)", filepath.c_str(), hr);
            throw std::runtime_error("Failed to load texture: " + filepath);
        }

        this->width = static_cast<uint32_t>(metadata.width);
        this->height = static_cast<uint32_t>(metadata.height);
        this->format = metadata.format;
        this->type = TextureType::Texture2D;

        Logger::Info("Texture info: %dx%d, format: %d, mipLevels: %zu",
            this->width, this->height, this->format, metadata.mipLevels);

        // テクスチャリソース作成（COPY_DEST状態で作成）
        CreateResource(
            device,
            this->width,
            this->height,
            this->format,
            D3D12_RESOURCE_FLAG_NONE,
            D3D12_RESOURCE_STATE_COPY_DEST,
            nullptr
        );

        // サブリソースデータの準備
        std::vector<D3D12_SUBRESOURCE_DATA> subresources;
        for (size_t i = 0; i < image.GetImageCount(); ++i) {
            const DirectX::Image* img = image.GetImage(0, i, 0);
            D3D12_SUBRESOURCE_DATA subresource = {};
            subresource.pData = img->pixels;
            subresource.RowPitch = img->rowPitch;
            subresource.SlicePitch = img->slicePitch;
            subresources.push_back(subresource);
        }

        // UploadContextを使ってGPUにアップロード
        // 注意: この実装ではUploadContextをここで作成・使用していますが、
        // 本来は外部から渡されるべきです（次のリファクタリングで改善）

        // 一時的な実装：後で外部から渡すように変更
        // uploadContext->Begin();
        // uploadContext->UploadTexture(resource.Get(), subresources.data(), subresources.size());
        // uploadContext->TransitionResource(resource.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        // uploadContext->End();

        Logger::Info("Texture loaded successfully: %s", filepath.c_str());
        Logger::Warning("Texture upload requires UploadContext (call UploadToGPU separately)");

        // 画像データを一時保存（UploadToGPU用）
        tempImageData = std::move(image);
    }

    void Texture::UploadToGPU(UploadContext* uploadContext) {
        if (!tempImageData.GetImageCount()) {
            Logger::Warning("No image data to upload");
            return;
        }

        // サブリソースデータの準備
        std::vector<D3D12_SUBRESOURCE_DATA> subresources;
        for (size_t i = 0; i < tempImageData.GetImageCount(); ++i) {
            const DirectX::Image* img = tempImageData.GetImage(0, i, 0);
            D3D12_SUBRESOURCE_DATA subresource = {};
            subresource.pData = img->pixels;
            subresource.RowPitch = static_cast<LONG_PTR>(img->rowPitch);
            subresource.SlicePitch = static_cast<LONG_PTR>(img->slicePitch);
            subresources.push_back(subresource);
        }

        // アップロード開始
        uploadContext->Begin();

        // テクスチャデータをアップロード
        uploadContext->UploadTexture(
            resource.Get(),
            subresources.data(),
            static_cast<uint32_t>(subresources.size())
        );

        // リソースバリア: COPY_DEST → PIXEL_SHADER_RESOURCE
        uploadContext->TransitionResource(
            resource.Get(),
            D3D12_RESOURCE_STATE_COPY_DEST,
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
        );

        // アップロード完了を待機
        uploadContext->End();

        // 一時データをクリア
        tempImageData.Release();

        Logger::Info("Texture uploaded to GPU successfully");
    }

    void Texture::CreateFromMemory(
        ID3D12Device* device,
        uint32_t width,
        uint32_t height,
        DXGI_FORMAT format,
        const void* data) {

        this->width = width;
        this->height = height;
        this->format = format;
        this->type = TextureType::Texture2D;

        CreateResource(
            device,
            width,
            height,
            format,
            D3D12_RESOURCE_FLAG_NONE,
            D3D12_RESOURCE_STATE_COPY_DEST,
            nullptr
        );

        // DirectXTexのScratchImageを作成
        DirectX::ScratchImage image;
        HRESULT hr = image.Initialize2D(format, width, height, 1, 1);
        if (FAILED(hr)) {
            throw std::runtime_error("Failed to initialize ScratchImage");
        }

        // データをコピー
        const DirectX::Image* img = image.GetImage(0, 0, 0);
        memcpy(img->pixels, data, img->slicePitch);

        // 一時保存
        tempImageData = std::move(image);

        Logger::Info("Texture created from memory: %dx%d", width, height);
    }

    void Texture::CreateRenderTarget(
        ID3D12Device* device,
        uint32_t width,
        uint32_t height,
        DXGI_FORMAT format) {

        this->width = width;
        this->height = height;
        this->format = format;
        this->type = TextureType::RenderTarget;

        D3D12_CLEAR_VALUE clearValue = {};
        clearValue.Format = format;
        clearValue.Color[0] = 0.0f;
        clearValue.Color[1] = 0.0f;
        clearValue.Color[2] = 0.0f;
        clearValue.Color[3] = 1.0f;

        CreateResource(
            device,
            width,
            height,
            format,
            D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET,
            D3D12_RESOURCE_STATE_RENDER_TARGET,
            &clearValue
        );

        Logger::Info("Render target texture created: %dx%d", width, height);
    }

    void Texture::CreateDepthStencil(
        ID3D12Device* device,
        uint32_t width,
        uint32_t height) {

        this->width = width;
        this->height = height;
        this->format = DXGI_FORMAT_D32_FLOAT;
        this->type = TextureType::DepthStencil;

        D3D12_CLEAR_VALUE clearValue = {};
        clearValue.Format = DXGI_FORMAT_D32_FLOAT;
        clearValue.DepthStencil.Depth = 1.0f;
        clearValue.DepthStencil.Stencil = 0;

        CreateResource(
            device,
            width,
            height,
            DXGI_FORMAT_D32_FLOAT,
            D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL,
            D3D12_RESOURCE_STATE_DEPTH_WRITE,
            &clearValue
        );

        Logger::Info("Depth stencil texture created: %dx%d", width, height);
    }

    void Texture::Shutdown() {
        tempImageData.Release();
        uploadBuffer.Reset();
        resource.Reset();
    }

    void Texture::CreateSRV(ID3D12Device* device, D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle) {
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = format;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Texture2D.MipLevels = 1;
        srvDesc.Texture2D.MostDetailedMip = 0;

        device->CreateShaderResourceView(resource.Get(), &srvDesc, cpuHandle);
    }

    void Texture::CreateRTV(ID3D12Device* device, D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle) {
        D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
        rtvDesc.Format = format;
        rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
        rtvDesc.Texture2D.MipSlice = 0;

        device->CreateRenderTargetView(resource.Get(), &rtvDesc, cpuHandle);
    }

    void Texture::CreateDSV(ID3D12Device* device, D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle) {
        D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
        dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
        dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
        dsvDesc.Texture2D.MipSlice = 0;

        device->CreateDepthStencilView(resource.Get(), &dsvDesc, cpuHandle);
    }

    void Texture::CreateResource(
        ID3D12Device* device,
        uint32_t width,
        uint32_t height,
        DXGI_FORMAT format,
        D3D12_RESOURCE_FLAGS flags,
        D3D12_RESOURCE_STATES initialState,
        const D3D12_CLEAR_VALUE* clearValue) {

        D3D12_RESOURCE_DESC resourceDesc = {};
        resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        resourceDesc.Width = width;
        resourceDesc.Height = height;
        resourceDesc.DepthOrArraySize = 1;
        resourceDesc.MipLevels = 1;
        resourceDesc.Format = format;
        resourceDesc.SampleDesc.Count = 1;
        resourceDesc.SampleDesc.Quality = 0;
        resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        resourceDesc.Flags = flags;

        D3D12_HEAP_PROPERTIES heapProps = {};
        heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

        HRESULT hr = device->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &resourceDesc,
            initialState,
            clearValue,
            IID_PPV_ARGS(&resource)
        );

        if (FAILED(hr)) {
            throw std::runtime_error("Failed to create texture resource");
        }
    }

} // namespace Athena