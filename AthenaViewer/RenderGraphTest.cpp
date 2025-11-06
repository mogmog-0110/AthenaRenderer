#include "Athena/RenderGraph/RenderGraph.h"
#include "Athena/RenderGraph/RenderGraphBuilder.h"
#include "Athena/Core/Device.h"
#include "Athena/Utils/Logger.h"
#include <memory>

using namespace Athena;

class TestRenderPass : public RenderPass {
public:
    TestRenderPass() : RenderPass("TestPass") {}
    
    void Setup(PassSetupData& setupData) override {
        Logger::Info("TestRenderPass::Setup called");
    }
    
    void Execute(const PassExecuteData& executeData) override {
        Logger::Info("TestRenderPass::Execute called");
    }
    
    std::string GetDescription() const override {
        return "Test render pass";
    }
    
    std::string GetCategory() const override {
        return "Test";
    }
};

bool TestRenderGraphBasics(std::shared_ptr<Device> device) {
    Logger::Info("=== RenderGraph Basic Test Start ===");
    
    try {
        RenderGraph graph(device);
        RenderGraphBuilder builder(&graph);
        
        auto colorTarget = builder.CreateColorTarget("TestColor", 1280, 720);
        auto depthTarget = builder.CreateDepthTarget("TestDepth", 1280, 720);
        
        Logger::Info("Resource creation completed:");
        Logger::Info("  - ColorTarget: {} (valid: {})", colorTarget.GetName(), colorTarget.IsValid());
        Logger::Info("  - DepthTarget: {} (valid: {})", depthTarget.GetName(), depthTarget.IsValid());
        
        builder.SetFinalOutput(colorTarget);
        
        bool compileResult = graph.Compile();
        Logger::Info("Compile result: {}", compileResult ? "SUCCESS" : "FAILED");
        
        const auto& stats = graph.GetStats();
        Logger::Info("Statistics:");
        Logger::Info("  - Total resources: {}", stats.totalResources);
        Logger::Info("  - Transient resources: {}", stats.transientResources);
        Logger::Info("  - External resources: {}", stats.externalResources);
        Logger::Info("  - Compile time: {:.3f}ms", stats.compileTime * 1000.0f);
        
        graph.DumpDebugInfo();
        
        std::string graphviz = graph.ExportGraphviz();
        Logger::Info("Graphviz output: {}", graphviz);
        
        Logger::Info("=== RenderGraph Basic Test Complete ===");
        return compileResult;
        
    } catch (const std::exception& e) {
        Logger::Error("RenderGraph test error: {}", e.what());
        return false;
    }
}

bool TestResourceHandle() {
    Logger::Info("=== ResourceHandle Test Start ===");
    
    auto desc = ResourceDesc::CreateTexture2D(
        1920, 1080, 
        DXGI_FORMAT_R8G8B8A8_UNORM,
        ResourceUsage::RenderTarget | ResourceUsage::ShaderResource,
        "TestTexture"
    );
    
    Logger::Info("ResourceDesc created:");
    Logger::Info("  - Size: {}x{}", desc.width, desc.height);
    Logger::Info("  - Format: {}", static_cast<int>(desc.format));
    Logger::Info("  - Name: {}", desc.debugName);
    
    bool hasRT = HasUsage(desc.usage, ResourceUsage::RenderTarget);
    bool hasSRV = HasUsage(desc.usage, ResourceUsage::ShaderResource);
    bool hasUAV = HasUsage(desc.usage, ResourceUsage::UnorderedAccess);
    
    Logger::Info("Usage flags:");
    Logger::Info("  - RenderTarget: {}", hasRT);
    Logger::Info("  - ShaderResource: {}", hasSRV);
    Logger::Info("  - UnorderedAccess: {}", hasUAV);
    
    Logger::Info("=== ResourceHandle Test Complete ===");
    return true;
}

bool TestRenderPassFunctionality() {
    Logger::Info("=== RenderPass Test Start ===");
    
    std::unique_ptr<TestRenderPass> testPass(new TestRenderPass());
    
    Logger::Info("Pass info:");
    Logger::Info("  - Name: {}", testPass->GetName());
    Logger::Info("  - Description: {}", testPass->GetDescription());
    Logger::Info("  - Category: {}", testPass->GetCategory());
    
    PassSetupData setupData;
    setupData.SetFloat("testFloat", 3.14f);
    setupData.SetInt("testInt", 42);
    setupData.SetBool("testBool", true);
    
    testPass->Setup(setupData);
    
    PassExecuteData executeData;
    executeData.floatParams = setupData.floatParams;
    executeData.intParams = setupData.intParams;
    executeData.boolParams = setupData.boolParams;
    
    Logger::Info("Parameter test:");
    Logger::Info("  - Float: {}", executeData.GetFloat("testFloat"));
    Logger::Info("  - Int: {}", executeData.GetInt("testInt"));
    Logger::Info("  - Bool: {}", executeData.GetBool("testBool"));
    
    testPass->Execute(executeData);
    
    Logger::Info("=== RenderPass Test Complete ===");
    return true;
}

bool RunAllRenderGraphTests(std::shared_ptr<Device> device) {
    Logger::Info("===== RenderGraph Integration Test Start =====");
    
    bool result = true;
    
    result &= TestResourceHandle();
    result &= TestRenderPassFunctionality();
    result &= TestRenderGraphBasics(device);
    
    Logger::Info("===== RenderGraph Integration Test Complete: {} =====", result ? "SUCCESS" : "FAILED");
    
    return result;
}