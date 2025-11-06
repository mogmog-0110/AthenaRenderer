#include "Athena/Resources/Texture.h"
#include "Athena/Resources/UploadContext.h"
#include "Athena/Utils/Logger.h"
#include <DirectXTex.h>
#include <stdexcept>
#include <algorithm>

namespace Athena {

    Texture::Texture() = default;

    Texture::~Texture() {
        Shutdown();
    }

    void Texture::LoadFromFile(
        ID3D12Device* device,
        const wchar_t* filepath,
        UploadContext* uploadContext,
        bool generateMips) {

        Logger::Info("Loading texture from file: %S", filepath);

        DirectX::ScratchImage scratchImage;
        HRESULT hr;

        std::wstring path(filepath);
        std::wstring ext = path.substr(path.find_last_of(L'.'));
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

        if (ext == L".dds") {
            hr = DirectX::LoadFromDDSFile(filepath, DirectX::DDS_FLAGS_NONE, nullptr, scratchImage);
        }
        else if (ext == L".tga") {
            hr = DirectX::LoadFromTGAFile(filepath, nullptr, scratchImage);
        }
        else if (ext == L".hdr") {
            hr = DirectX::LoadFromHDRFile(filepath, nullptr, scratchImage);
        }
        else {
            hr = DirectX::LoadFromWICFile(filepath, DirectX::WIC_FLAGS_NONE, nullptr, scratchImage);
        }

        if (FAILED(hr)) {
            Logger::Error("Failed to load texture file: %S", filepath);
            throw std::runtime_error("Failed to load texture file");
        }

        const DirectX::TexMetadata& metadata = scratchImage.GetMetadata();
        width = static_cast<uint32_t>(metadata.width);
        height = static_cast<uint32_t>(metadata.height);
        format = metadata.format;

        Logger::Info("  Size: %ux%u", width, height);
        Logger::Info("  Format: %d", format);
        Logger::Info("  Mip levels in file: %zu", metadata.mipLevels);

        DirectX::ScratchImage mipChain;
        if (generateMips && metadata.mipLevels == 1) {
            Logger::Info("  Generating mipmaps...");
            hr = DirectX::GenerateMipMaps(
                scratchImage.GetImages(),
                scratchImage.GetImageCount(),
                scratchImage.GetMetadata(),
                DirectX::TEX_FILTER_DEFAULT,
                0,
                mipChain
            );

            if (SUCCEEDED(hr)) {
                scratchImage = std::move(mipChain);
                mipLevels = static_cast<uint32_t>(scratchImage.GetMetadata().mipLevels);
                Logger::Info("  Generated %u mip levels", mipLevels);
            }
            else {
                Logger::Warning("  Failed to generate mipmaps, using original image");
                mipLevels = 1;
            }
        }
        else {
            mipLevels = static_cast<uint32_t>(metadata.mipLevels);
        }

        D3D12_RESOURCE_DESC textureDesc = {};
        textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        textureDesc.Width = width;
        textureDesc.Height = height;
        textureDesc.DepthOrArraySize = 1;
        textureDesc.MipLevels = static_cast<UINT16>(mipLevels);
        textureDesc.Format = format;
        textureDesc.SampleDesc.Count = 1;
        textureDesc.SampleDesc.Quality = 0;
        textureDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

        D3D12_HEAP_PROPERTIES heapProps = {};
        heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

        hr = device->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &textureDesc,
            D3D12_RESOURCE_STATE_COPY_DEST,
            nullptr,
            IID_PPV_ARGS(&resource)
        );

        if (FAILED(hr)) {
            throw std::runtime_error("Failed to create texture resource");
        }

        std::vector<D3D12_SUBRESOURCE_DATA> subresources(mipLevels);
        const DirectX::Image* images = scratchImage.GetImages();

        for (uint32_t i = 0; i < mipLevels; ++i) {
            subresources[i].pData = images[i].pixels;
            subresources[i].RowPitch = images[i].rowPitch;
            subresources[i].SlicePitch = images[i].slicePitch;
        }

        uploadContext->Begin();
        uploadContext->UploadTexture(resource.Get(), subresources.data(), mipLevels);

        uploadContext->TransitionResource(
            resource.Get(),
            D3D12_RESOURCE_STATE_COPY_DEST,
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
        );

        uploadContext->End();

        Logger::Info("✓ Texture loaded successfully: %S", filepath);
    }

    void Texture::UploadToGPU(UploadContext* uploadContext) {
        if (!tempImageData.GetImageCount()) {
            Logger::Warning("No image data to upload");
            return;
        }

        std::vector<D3D12_SUBRESOURCE_DATA> subresources;
        for (size_t i = 0; i < tempImageData.GetImageCount(); ++i) {
            const DirectX::Image* img = tempImageData.GetImage(0, i, 0);
            D3D12_SUBRESOURCE_DATA subresource = {};
            subresource.pData = img->pixels;
            subresource.RowPitch = static_cast<LONG_PTR>(img->rowPitch);
            subresource.SlicePitch = static_cast<LONG_PTR>(img->slicePitch);
            subresources.push_back(subresource);
        }

        uploadContext->Begin();

        uploadContext->UploadTexture(
            resource.Get(),
            subresources.data(),
            static_cast<uint32_t>(subresources.size())
        );

        uploadContext->TransitionResource(
            resource.Get(),
            D3D12_RESOURCE_STATE_COPY_DEST,
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
        );

        uploadContext->End();

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

        DirectX::ScratchImage image;
        HRESULT hr = image.Initialize2D(format, width, height, 1, 1);
        if (FAILED(hr)) {
            throw std::runtime_error("Failed to initialize ScratchImage");
        }

        const DirectX::Image* img = image.GetImage(0, 0, 0);
        memcpy(img->pixels, data, img->slicePitch);

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
        this->format = DXGI_FORMAT_R32_TYPELESS;  // SRVとして読み取り可能にする
        this->type = TextureType::DepthStencil;

        D3D12_CLEAR_VALUE clearValue = {};
        clearValue.Format = DXGI_FORMAT_D32_FLOAT;  // DSVフォーマット
        clearValue.DepthStencil.Depth = 1.0f;
        clearValue.DepthStencil.Stencil = 0;

        CreateResource(
            device,
            width,
            height,
            DXGI_FORMAT_R32_TYPELESS,  // リソースはTypelessで作成
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
        
        // 深度テクスチャの場合は R32_FLOAT フォーマットを使用
        if (type == TextureType::DepthStencil && format == DXGI_FORMAT_R32_TYPELESS) {
            srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
        } else {
            srvDesc.Format = format;
        }
        
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