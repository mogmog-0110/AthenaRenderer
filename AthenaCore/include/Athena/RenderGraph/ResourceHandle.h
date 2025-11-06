#pragma once
#include <string>
#include <memory>
#include <d3d12.h>
#include <dxgi1_6.h>

namespace Athena {

    // Forward declarations
    class Texture;
    class Buffer;

    /**
     * @brief リソースタイプ列挙
     */
    enum class ResourceType {
        Texture2D,      // 2Dテクスチャ
        TextureCube,    // キューブテクスチャ
        Texture3D,      // 3Dテクスチャ
        Buffer,         // バッファ
        Unknown         // 不明
    };

    /**
     * @brief リソースの使用方法フラグ
     */
    enum class ResourceUsage : uint32_t {
        None = 0,
        RenderTarget = 1 << 0,      // レンダーターゲットとして使用
        DepthStencil = 1 << 1,      // デプスステンシルとして使用
        ShaderResource = 1 << 2,    // シェーダーリソースとして使用
        UnorderedAccess = 1 << 3,   // UAVとして使用
        CopySource = 1 << 4,        // コピー元として使用
        CopyDestination = 1 << 5,   // コピー先として使用
    };

    // ビット演算子のオーバーロード
    inline ResourceUsage operator|(ResourceUsage a, ResourceUsage b) {
        return static_cast<ResourceUsage>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
    }

    inline ResourceUsage operator&(ResourceUsage a, ResourceUsage b) {
        return static_cast<ResourceUsage>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
    }

    inline bool HasUsage(ResourceUsage usage, ResourceUsage flag) {
        return (usage & flag) == flag;
    }

    /**
     * @brief リソースの詳細情報
     */
    struct ResourceDesc {
        ResourceType type = ResourceType::Unknown;
        ResourceUsage usage = ResourceUsage::None;
        uint32_t width = 0;
        uint32_t height = 0;
        uint32_t depth = 1;
        uint32_t mipLevels = 1;
        uint32_t arraySize = 1;
        DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
        std::string debugName;

        // テクスチャ用コンストラクタ
        static ResourceDesc CreateTexture2D(
            uint32_t width,
            uint32_t height,
            DXGI_FORMAT format,
            ResourceUsage usage,
            const std::string& name = "",
            uint32_t mipLevels = 1
        ) {
            ResourceDesc desc;
            desc.type = ResourceType::Texture2D;
            desc.width = width;
            desc.height = height;
            desc.format = format;
            desc.usage = usage;
            desc.debugName = name;
            desc.mipLevels = mipLevels;
            return desc;
        }

        // バッファ用コンストラクタ
        static ResourceDesc CreateBuffer(
            uint32_t size,
            ResourceUsage usage,
            const std::string& name = ""
        ) {
            ResourceDesc desc;
            desc.type = ResourceType::Buffer;
            desc.width = size;
            desc.usage = usage;
            desc.debugName = name;
            return desc;
        }
    };

    /**
     * @brief RenderGraph内でリソースを参照するためのハンドル
     * 
     * ResourceHandleは実際のリソース（TextureやBuffer）への軽量な参照である。
     * RenderGraphがリソースのライフタイムを管理し、必要に応じて
     * 一時的なリソースを作成・破棄する。
     */
    class ResourceHandle {
    public:
        ResourceHandle() = default;
        
        /**
         * @brief 無効なハンドルかチェック
         */
        bool IsValid() const { return id != INVALID_ID; }

        /**
         * @brief ハンドルをリセット（無効にする）
         */
        void Reset() { id = INVALID_ID; name.clear(); desc = ResourceDesc{}; }

        /**
         * @brief リソース名を取得
         */
        const std::string& GetName() const { return name; }

        /**
         * @brief リソース詳細を取得
         */
        const ResourceDesc& GetDesc() const { return desc; }

        /**
         * @brief 内部IDを取得（デバッグ用）
         */
        uint32_t GetID() const { return id; }

        /**
         * @brief 比較演算子
         */
        bool operator==(const ResourceHandle& other) const {
            return id == other.id;
        }

        bool operator!=(const ResourceHandle& other) const {
            return !(*this == other);
        }

        /**
         * @brief ハッシュ関数用
         */
        size_t Hash() const {
            return std::hash<uint32_t>{}(id);
        }

    private:
        friend class RenderGraph;
        friend class RenderGraphBuilder;
        friend class ExternalResourceHandle;
        
        static constexpr uint32_t INVALID_ID = 0xFFFFFFFF;
        
        uint32_t id = INVALID_ID;
        std::string name;
        ResourceDesc desc;

        /**
         * @brief 内部コンストラクタ（RenderGraphからのみ使用）
         */
        ResourceHandle(uint32_t resourceId, const std::string& resourceName, const ResourceDesc& resourceDesc)
            : id(resourceId), name(resourceName), desc(resourceDesc) {}
    };

    /**
     * @brief 外部リソース（既存のTexture/Buffer）を参照するハンドル
     */
    class ExternalResourceHandle : public ResourceHandle {
    public:
        ExternalResourceHandle() = default;
        
        /**
         * @brief 外部テクスチャからハンドルを作成
         */
        static ExternalResourceHandle FromTexture(std::shared_ptr<Texture> texture);
        
        /**
         * @brief 外部バッファからハンドルを作成
         */
        static ExternalResourceHandle FromBuffer(std::shared_ptr<Buffer> buffer);

        /**
         * @brief 参照している実際のリソースを取得
         */
        std::shared_ptr<Texture> GetTexture() const { return textureResource; }
        std::shared_ptr<Buffer> GetBuffer() const { return bufferResource; }

    private:
        std::shared_ptr<Texture> textureResource;
        std::shared_ptr<Buffer> bufferResource;
    };

} // namespace Athena

// std::hash特殊化
namespace std {
    template<>
    struct hash<Athena::ResourceHandle> {
        size_t operator()(const Athena::ResourceHandle& handle) const {
            return handle.Hash();
        }
    };
}