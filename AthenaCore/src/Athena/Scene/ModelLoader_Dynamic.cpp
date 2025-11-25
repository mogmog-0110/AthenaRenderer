#include "Athena/Scene/ModelLoader.h"
#include "Athena/Core/Device.h"
#include "Athena/Utils/Logger.h"
#include <Windows.h>
#include <filesystem>

namespace Athena {

// 動的ローディング用の関数ポインタ
class AssimpDynamicLoader {
private:
    static HMODULE assimpDll;
    
    // Assimp C API関数ポインタ型定義
    typedef const void* (*aiImportFileFunc)(const char*, unsigned int);
    typedef const char* (*aiGetErrorStringFunc)();
    typedef int (*aiGetVersionMajorFunc)();
    typedef void (*aiReleaseImportFunc)(const void*);
    
    // 関数ポインタ
    static aiImportFileFunc aiImportFile;
    static aiGetErrorStringFunc aiGetErrorString;
    static aiGetVersionMajorFunc aiGetVersionMajor;
    static aiReleaseImportFunc aiReleaseImport;
    
public:
    static bool Initialize() {
        if (assimpDll) return true; // 既に初期化済み
        
        // DLLの読み込み
        assimpDll = LoadLibrary(L"assimp-vc143-mt.dll");
        if (!assimpDll) {
            Logger::Error("Failed to load assimp-vc143-mt.dll");
            return false;
        }
        
        // 関数の取得
        aiImportFile = (aiImportFileFunc)GetProcAddress(assimpDll, "aiImportFile");
        aiGetErrorString = (aiGetErrorStringFunc)GetProcAddress(assimpDll, "aiGetErrorString");
        aiGetVersionMajor = (aiGetVersionMajorFunc)GetProcAddress(assimpDll, "aiGetVersionMajor");
        aiReleaseImport = (aiReleaseImportFunc)GetProcAddress(assimpDll, "aiReleaseImport");
        
        if (!aiImportFile || !aiGetErrorString || !aiGetVersionMajor || !aiReleaseImport) {
            Logger::Error("Failed to get required Assimp functions");
            FreeLibrary(assimpDll);
            assimpDll = nullptr;
            return false;
        }
        
        Logger::Info("Assimp {}.x loaded successfully via dynamic loading", aiGetVersionMajor());
        return true;
    }
    
    static const void* ImportFile(const char* filepath, unsigned int flags) {
        if (!Initialize()) return nullptr;
        return aiImportFile(filepath, flags);
    }
    
    static const char* GetErrorString() {
        if (!Initialize()) return "Dynamic loading failed";
        return aiGetErrorString();
    }
    
    static void ReleaseImport(const void* scene) {
        if (!Initialize() || !scene) return;
        aiReleaseImport(scene);
    }
    
    static void Cleanup() {
        if (assimpDll) {
            FreeLibrary(assimpDll);
            assimpDll = nullptr;
        }
    }
};

// 静的メンバの定義
HMODULE AssimpDynamicLoader::assimpDll = nullptr;
AssimpDynamicLoader::aiImportFileFunc AssimpDynamicLoader::aiImportFile = nullptr;
AssimpDynamicLoader::aiGetErrorStringFunc AssimpDynamicLoader::aiGetErrorString = nullptr;
AssimpDynamicLoader::aiGetVersionMajorFunc AssimpDynamicLoader::aiGetVersionMajor = nullptr;
AssimpDynamicLoader::aiReleaseImportFunc AssimpDynamicLoader::aiReleaseImport = nullptr;

// ModelLoaderの実装

std::vector<std::string> ModelLoader::GetSupportedFormats() {
    return {
        ".obj", ".fbx", ".dae", ".gltf", ".glb", 
        ".3ds", ".blend", ".x", ".md5", ".ply",
        ".stl", ".ase", ".ifc"
    };
}

std::shared_ptr<Model> ModelLoader::LoadModel(
    std::shared_ptr<Device> device,
    const std::string& filepath,
    bool flipUV,
    bool generateTangents) {
    
    Logger::Info("Loading model with dynamic Assimp: {}", filepath);
    
    // ファイルの存在確認
    if (!std::filesystem::exists(filepath)) {
        Logger::Error("Model file not found: {}", filepath);
        return nullptr;
    }
    
    // Assimp処理フラグ
    // aiProcess_Triangulate = 0x8
    // aiProcess_JoinIdenticalVertices = 0x2
    // aiProcess_GenNormals = 0x20
    // aiProcess_CalcTangentSpace = 0x1
    // aiProcess_FlipUVs = 0x800000
    unsigned int flags = 0x8 | 0x2 | 0x20;
    
    if (generateTangents) {
        flags |= 0x1;
    }
    
    if (flipUV) {
        flags |= 0x800000;
    }
    
    // モデルの読み込み（動的ローディング）
    const void* scene = AssimpDynamicLoader::ImportFile(filepath.c_str(), flags);
    
    if (!scene) {
        Logger::Error("Failed to load model: {}", AssimpDynamicLoader::GetErrorString());
        return nullptr;
    }
    
    // Modelオブジェクトの作成
    auto model = std::make_shared<Model>();
    model->filename = std::filesystem::path(filepath).filename().string();
    model->directory = std::filesystem::path(filepath).parent_path().string();
    
    // 基本的な情報のみ設定（実際のメッシュ解析は後で実装）
    Logger::Info("Model file loaded successfully: {}", model->filename);
    Logger::Info("Note: Full mesh parsing requires complete Assimp headers");
    Logger::Info("Current implementation: Dynamic loading verification only");
    
    // シーン構造体の詳細解析は完全なヘッダーファイルが必要
    // 現時点では成功の確認のみ
    
    // リソースの解放
    AssimpDynamicLoader::ReleaseImport(scene);
    
    return model;
}

// クリーンアップ用（アプリケーション終了時に呼び出し）
void ModelLoader::Cleanup() {
    AssimpDynamicLoader::Cleanup();
}

} // namespace Athena