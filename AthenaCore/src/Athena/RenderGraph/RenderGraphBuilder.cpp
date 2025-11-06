#include "Athena/RenderGraph/RenderGraphBuilder.h"
#include "Athena/Resources/Texture.h"
#include "Athena/Resources/Buffer.h"
#include "Athena/Utils/Logger.h"

namespace Athena {

    RenderGraphBuilder::RenderGraphBuilder(RenderGraph* graph) 
        : graph(graph) {
        if (!graph) {
            Logger::Error("RenderGraphBuilder created with null RenderGraph");
        }
    }

    ResourceHandle RenderGraphBuilder::CreateTexture(
        const std::string& name,
        uint32_t width,
        uint32_t height,
        DXGI_FORMAT format,
        ResourceUsage usage) {
        
        if (!graph) {
            Logger::Error("Cannot create texture: RenderGraph is null");
            return ResourceHandle{};
        }

        ResourceDesc desc = ResourceDesc::CreateTexture2D(width, height, format, usage, name);
        ResourceHandle handle = graph->CreateResource(desc, name);
        
        namedResources[name] = handle;
        
        Logger::Info("Created texture resource: {} ({}x{}, format={})", 
            name, width, height, static_cast<int>(format));
        
        return handle;
    }

    ResourceHandle RenderGraphBuilder::CreateBuffer(
        const std::string& name,
        uint32_t size,
        ResourceUsage usage) {
        
        if (!graph) {
            Logger::Error("Cannot create buffer: RenderGraph is null");
            return ResourceHandle{};
        }

        ResourceDesc desc = ResourceDesc::CreateBuffer(size, usage, name);
        ResourceHandle handle = graph->CreateResource(desc, name);
        
        namedResources[name] = handle;
        
        Logger::Info("Created buffer resource: {} ({} bytes)", name, size);
        
        return handle;
    }

    ResourceHandle RenderGraphBuilder::ImportTexture(
        const std::string& name,
        std::shared_ptr<Texture> texture) {
        
        if (!graph || !texture) {
            Logger::Error("Cannot import texture: RenderGraph or texture is null");
            return ResourceHandle{};
        }

        ExternalResourceHandle extHandle = ExternalResourceHandle::FromTexture(texture);
        ResourceHandle handle;
        handle.id = 0;
        handle.name = name;
        handle.desc = extHandle.GetDesc();
        
        graph->RegisterExternalResource(handle, texture);
        
        namedResources[name] = handle;
        
        Logger::Info("Imported external texture: {}", name);
        
        return handle;
    }

    ResourceHandle RenderGraphBuilder::ImportBuffer(
        const std::string& name,
        std::shared_ptr<Buffer> buffer) {
        
        if (!graph || !buffer) {
            Logger::Error("Cannot import buffer: RenderGraph or buffer is null");
            return ResourceHandle{};
        }

        ExternalResourceHandle extHandle = ExternalResourceHandle::FromBuffer(buffer);
        ResourceHandle handle;
        handle.id = 0;
        handle.name = name;
        handle.desc = extHandle.GetDesc();
        
        graph->RegisterExternalResource(handle, buffer);
        
        namedResources[name] = handle;
        
        Logger::Info("Imported external buffer: {}", name);
        
        return handle;
    }

    RenderGraphBuilder& RenderGraphBuilder::Read(const ResourceHandle& handle, const std::string& passName) {
        if (!handle.IsValid()) {
            Logger::Error("Cannot add read dependency: invalid resource handle");
            return *this;
        }

        std::string actualPassName = passName.empty() ? currentPassName : passName;
        readDependencies.emplace_back(handle, actualPassName);
        
        Logger::Info("Added read dependency: {} -> {}", handle.GetName(), actualPassName);
        
        return *this;
    }

    RenderGraphBuilder& RenderGraphBuilder::Write(const ResourceHandle& handle, const std::string& passName) {
        if (!handle.IsValid()) {
            Logger::Error("Cannot add write dependency: invalid resource handle");
            return *this;
        }

        std::string actualPassName = passName.empty() ? currentPassName : passName;
        writeDependencies.emplace_back(handle, actualPassName);
        
        Logger::Info("Added write dependency: {} -> {}", handle.GetName(), actualPassName);
        
        return *this;
    }

    RenderGraphBuilder& RenderGraphBuilder::ReadWrite(const ResourceHandle& handle, const std::string& passName) {
        Read(handle, passName);
        Write(handle, passName);
        return *this;
    }

    RenderGraphBuilder& RenderGraphBuilder::AddInput(const std::string& name, const ResourceHandle& handle) {
        if (!handle.IsValid()) {
            Logger::Error("Cannot add input: invalid resource handle for '{}'", name);
            return *this;
        }

        currentInputs[name] = handle;
        Logger::Info("Added input '{}' -> {}", name, handle.GetName());
        
        return *this;
    }

    RenderGraphBuilder& RenderGraphBuilder::AddOutput(const std::string& name, const ResourceHandle& handle) {
        if (!handle.IsValid()) {
            Logger::Error("Cannot add output: invalid resource handle for '{}'", name);
            return *this;
        }

        currentOutputs[name] = handle;
        Logger::Info("Added output '{}' -> {}", name, handle.GetName());
        
        return *this;
    }

    RenderGraphBuilder& RenderGraphBuilder::SetFloat(const std::string& name, float value) {
        currentFloatParams[name] = value;
        return *this;
    }

    RenderGraphBuilder& RenderGraphBuilder::SetInt(const std::string& name, int value) {
        currentIntParams[name] = value;
        return *this;
    }

    RenderGraphBuilder& RenderGraphBuilder::SetBool(const std::string& name, bool value) {
        currentBoolParams[name] = value;
        return *this;
    }

    RenderGraphBuilder& RenderGraphBuilder::SetFinalOutput(const ResourceHandle& handle) {
        if (!graph) {
            Logger::Error("Cannot set final output: RenderGraph is null");
            return *this;
        }

        graph->SetFinalOutput(handle);
        return *this;
    }

    // 便利なリソース作成メソッド

    ResourceHandle RenderGraphBuilder::CreateColorTarget(
        const std::string& name, 
        uint32_t width, 
        uint32_t height, 
        DXGI_FORMAT format) {
        
        return CreateTexture(name, width, height, format, 
            ResourceUsage::RenderTarget | ResourceUsage::ShaderResource);
    }

    ResourceHandle RenderGraphBuilder::CreateDepthTarget(
        const std::string& name, 
        uint32_t width, 
        uint32_t height, 
        DXGI_FORMAT format) {
        
        return CreateTexture(name, width, height, format, 
            ResourceUsage::DepthStencil | ResourceUsage::ShaderResource);
    }

    ResourceHandle RenderGraphBuilder::CreateConstantBuffer(
        const std::string& name, 
        uint32_t size) {
        
        return CreateBuffer(name, size, ResourceUsage::ShaderResource);
    }

    ResourceHandle RenderGraphBuilder::CreateStructuredBuffer(
        const std::string& name, 
        uint32_t elementSize, 
        uint32_t elementCount) {
        
        uint32_t totalSize = elementSize * elementCount;
        return CreateBuffer(name, totalSize, 
            ResourceUsage::ShaderResource | ResourceUsage::UnorderedAccess);
    }

    // プライベートメソッド

    void RenderGraphBuilder::BeginPass(const std::string& passName) {
        if (inPassContext) {
            Logger::Warning("Beginning new pass '{}' while still in pass context '{}'", 
                passName, currentPassName);
            EndPass();
        }

        inPassContext = true;
        currentPassName = passName;
        ClearCurrentPass();
        
        Logger::Info("Began pass context: {}", passName);
    }

    void RenderGraphBuilder::EndPass() {
        if (!inPassContext) {
            Logger::Warning("EndPass called without active pass context");
            return;
        }

        // 現在のパス情報をRenderGraphに適用
        
        Logger::Info("Ended pass context: {}", currentPassName);
        
        inPassContext = false;
        currentPassName.clear();
        ClearCurrentPass();
    }

    void RenderGraphBuilder::ClearCurrentPass() {
        currentInputs.clear();
        currentOutputs.clear();
        currentFloatParams.clear();
        currentIntParams.clear();
        currentBoolParams.clear();
    }

} // namespace Athena