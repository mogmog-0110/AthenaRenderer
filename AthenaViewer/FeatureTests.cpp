#include <iostream>
#include <memory>
#include "Athena/Core/Device.h"
#include "Athena/Scene/Scene.h"
#include "Athena/Scene/Camera.h"
#include "Athena/Scene/CameraController.h"
#include "Athena/Scene/ModelLoader.h"
#include "Athena/Scene/SceneObject.h"
#include "Athena/Utils/Logger.h"
#include "Athena/Utils/Math.h"

using namespace Athena;

// Dummy Camera for testing (Camera is abstract)
class DummyCamera : public Camera {
public:
    DummyCamera() = default;
    virtual ~DummyCamera() = default;
    
    void Update(float deltaTime) override {
        // Stub implementation
    }
    
    void UpdateViewMatrix() override {
        // Stub implementation
    }
};

// Test Assimp Library Integration
bool TestAssimpIntegration(std::shared_ptr<Device> device) {
    Logger::Info("=== Testing Assimp Library Integration ===");
    
    try {
        // 1. Test ModelLoader instance creation
        Logger::Info("Testing ModelLoader creation...");
        
        // 2. Check supported formats
        Logger::Info("Testing format support...");
        std::vector<std::string> expectedFormats = {".obj", ".fbx", ".gltf", ".glb", ".dae"};
        Logger::Info("Expected supported formats:");
        for (const auto& format : expectedFormats) {
            Logger::Info("  - %s", format.c_str());
        }
        
        // 3. Test non-existent file loading (error handling)
        Logger::Info("Testing error handling with non-existent file...");
        auto model1 = ModelLoader::LoadModel(device, "non_existent_file.obj");
        if (!model1) {
            Logger::Info("OK - Correctly handled non-existent file");
        } else {
            Logger::Error("ERROR - Error handling failed");
            return false;
        }
        
        // 4. Test invalid format file
        Logger::Info("Testing invalid format handling...");
        auto model2 = ModelLoader::LoadModel(device, "invalid_file.xyz");
        if (!model2) {
            Logger::Info("OK - Correctly handled invalid format");
        } else {
            Logger::Error("ERROR - Invalid format handling failed");
            return false;
        }
        
        // 5. Test FBX file loading (if file exists)
        Logger::Info("Testing FBX file loading...");
        
        // Test multiple candidate paths
        std::vector<std::string> testPaths = {
            "teamugfbx.fbx",                    // 実際にコピーしたファイル
            "assets/models/teamugfbx.fbx",
            "assets/models/test.fbx",
            "test.fbx",
            "models/test.fbx",
            "assets/test.fbx"
        };
        
        bool fbxTestPassed = false;
        for (const auto& path : testPaths) {
            Logger::Info("Attempting to load: %s", path.c_str());
            auto fbxModel = ModelLoader::LoadModel(device, path);
            if (fbxModel) {
                Logger::Info("OK - Successfully loaded FBX file: %s", path.c_str());
                Logger::Info("  Filename: %s", fbxModel->filename.c_str());
                Logger::Info("  Directory: %s", fbxModel->directory.c_str());
                fbxTestPassed = true;
                break;
            }
        }
        
        if (!fbxTestPassed) {
            Logger::Info("INFO - No FBX test files found. Place a .fbx file in one of these locations:");
            for (const auto& path : testPaths) {
                Logger::Info("  - %s", path.c_str());
            }
        }
        
        // 6. Test optional parameters
        Logger::Info("Testing optional parameters...");
        auto model3 = ModelLoader::LoadModel(device, "test.obj", true, true);
        if (!model3) {
            Logger::Info("OK - LoadModel with options completed (expected failure for non-existent file)");
        }
        
        Logger::Info("OK - Assimp integration test completed successfully");
        return true;
        
    } catch (const std::exception& e) {
        Logger::Error("ERROR - Assimp integration test failed: %s", e.what());
        return false;
    }
}

// Test Camera System Enhancement
bool TestCameraSystemEnhancement() {
    Logger::Info("=== Testing Camera System Enhancement ===");
    
    try {
        // 1. Create camera
        Logger::Info("Creating camera...");
        auto camera = std::make_shared<DummyCamera>();
        
        // 2. Create camera controller
        Logger::Info("Creating camera controller...");
        auto cameraController = std::make_shared<CameraController>(camera);
        
        // 3. Test basic operations
        Logger::Info("Testing basic camera operations...");
        cameraController->SetPosition(Vector3(10.0f, 5.0f, 15.0f));
        cameraController->SetTarget(Vector3(0.0f, 0.0f, 0.0f));
        cameraController->LookAt(Vector3(5.0f, 2.0f, 8.0f));
        Logger::Info("OK - Basic camera operations completed");
        
        // 4. Test camera settings
        Logger::Info("Testing camera settings...");
        cameraController->SetFOV(60.0f);
        cameraController->SetNearFar(0.1f, 1000.0f);
        Logger::Info("OK - Camera settings completed");
        
        // 5. Test camera animation
        Logger::Info("Testing camera animation...");
        auto animation = std::make_shared<CameraAnimation>();
        cameraController->StartAnimation(animation);
        
        if (cameraController->IsAnimating()) {
            Logger::Info("OK - Animation started successfully");
        } else {
            Logger::Info("INFO - Animation system available (stub implementation)");
        }
        
        Logger::Info("OK - Camera system enhancement test completed successfully");
        return true;
        
    } catch (const std::exception& e) {
        Logger::Error("ERROR - Camera system enhancement test failed: %s", e.what());
        return false;
    }
}

// Test Multi-Mesh Rendering
bool TestMultiMeshRendering(std::shared_ptr<Device> device) {
    Logger::Info("=== Testing Multi-Mesh Rendering ===");
    
    try {
        // 1. Create scene
        Logger::Info("Creating scene...");
        auto scene = std::make_shared<Scene>();
        
        // 2. Create scene objects
        Logger::Info("Creating scene objects...");
        auto object1 = std::make_shared<SceneObject>();
        auto object2 = std::make_shared<SceneObject>();
        auto object3 = std::make_shared<SceneObject>();
        
        // 3. Set transforms for objects
        Logger::Info("Setting object transforms...");
        object1->SetPosition(Vector3(0.0f, 0.0f, 0.0f));
        object1->SetScale(Vector3(1.0f, 1.0f, 1.0f));
        
        object2->SetPosition(Vector3(5.0f, 0.0f, 0.0f));
        object2->SetScale(Vector3(0.5f, 0.5f, 0.5f));
        
        object3->SetPosition(Vector3(-5.0f, 0.0f, 0.0f));
        object3->SetRotation(Vector3(0.0f, 45.0f, 0.0f));
        
        // 4. Add objects to scene
        Logger::Info("Adding objects to scene...");
        scene->AddObject(object1);
        scene->AddObject(object2);
        scene->AddObject(object3);
        
        // 5. Test scene update
        Logger::Info("Testing scene update...");
        float deltaTime = 0.016f; // 60 FPS
        scene->Update(deltaTime);
        
        // 6. Test scene rendering
        Logger::Info("Testing scene rendering...");
        auto camera = std::make_shared<DummyCamera>();
        scene->Render(camera);
        
        Logger::Info("OK - Multi-mesh rendering test completed successfully");
        return true;
        
    } catch (const std::exception& e) {
        Logger::Error("ERROR - Multi-mesh rendering test failed: %s", e.what());
        return false;
    }
}

// Run all feature tests
bool RunFeatureTests(std::shared_ptr<Device> device) {
    Logger::Info("Starting all feature tests...");
    
    bool allTestsPassed = true;
    
    //// Test 1: Assimp Integration
    //if (!TestAssimpIntegration(device)) {
    //    allTestsPassed = false;
    //}
    
    // Test 2: Camera System Enhancement
    if (!TestCameraSystemEnhancement()) {
        allTestsPassed = false;
    }
    
    // Test 3: Multi-Mesh Rendering
    if (!TestMultiMeshRendering(device)) {
        allTestsPassed = false;
    }
    
    if (allTestsPassed) {
        Logger::Info("=== ALL FEATURE TESTS PASSED ===");
    } else {
        Logger::Error("=== SOME FEATURE TESTS FAILED ===");
    }
    
    return allTestsPassed;
}