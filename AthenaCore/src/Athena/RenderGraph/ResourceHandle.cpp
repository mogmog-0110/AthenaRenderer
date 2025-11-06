#include "Athena/RenderGraph/ResourceHandle.h"
#include "Athena/Resources/Texture.h"
#include "Athena/Resources/Buffer.h"

namespace Athena {

    ExternalResourceHandle ExternalResourceHandle::FromTexture(std::shared_ptr<Texture> texture) {
        if (!texture) {
            return ExternalResourceHandle{};
        }

        ExternalResourceHandle handle;
        handle.textureResource = texture;
        
        // TextureからResourceDescを構築
        ResourceDesc desc;
        desc.type = ResourceType::Texture2D;
        desc.width = texture->GetWidth();
        desc.height = texture->GetHeight();
        desc.format = texture->GetFormat();
        desc.debugName = "ExternalTexture";
        
        // テクスチャの用途を推定
        desc.usage = ResourceUsage::ShaderResource;
        if (texture->IsRenderTarget()) {
            desc.usage = desc.usage | ResourceUsage::RenderTarget;
        }
        if (texture->IsDepthStencil()) {
            desc.usage = desc.usage | ResourceUsage::DepthStencil;
        }

        handle.desc = desc;
        handle.name = desc.debugName;
        handle.id = 0; // 外部リソースは特別なID
        
        return handle;
    }

    ExternalResourceHandle ExternalResourceHandle::FromBuffer(std::shared_ptr<Buffer> buffer) {
        if (!buffer) {
            return ExternalResourceHandle{};
        }

        ExternalResourceHandle handle;
        handle.bufferResource = buffer;
        
        // BufferからResourceDescを構築
        ResourceDesc desc;
        desc.type = ResourceType::Buffer;
        desc.width = buffer->GetSize();
        desc.debugName = "ExternalBuffer";
        desc.usage = ResourceUsage::ShaderResource; // デフォルトはSRV
        
        handle.desc = desc;
        handle.name = desc.debugName;
        handle.id = 0; // 外部リソースは特別なID
        
        return handle;
    }

} // namespace Athena