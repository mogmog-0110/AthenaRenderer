#include "Athena/Scene/ModelLoader.h"
#include "Athena/Core/Device.h"
#include "Athena/Utils/Logger.h"
#include "Athena/Utils/Math.h"

// Assimp includes
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <filesystem>
#include <algorithm>

namespace Athena {

std::shared_ptr<Model> ModelLoader::LoadModel(
    std::shared_ptr<Device> device,
    const std::string& filepath,
    bool flipUV,
    bool generateTangents) {
    
    Logger::Info("Loading model with Assimp: %s", filepath.c_str());
    
    // ファイル存在チェックの詳細ログを追加
    std::error_code ec;
    bool fileExists = std::filesystem::exists(filepath, ec);
    
    if (ec) {
        Logger::Error("Filesystem error when checking file: %s (error: %s)", filepath.c_str(), ec.message().c_str());
        return nullptr;
    }
    
    if (!fileExists) {
        Logger::Error("Model file not found: %s", filepath.c_str());
        // 現在の作業ディレクトリも表示
        std::error_code pathEc;
        auto currentPathObj = std::filesystem::current_path(pathEc);
        if (!pathEc) {
            std::string currentPath = currentPathObj.string();
            Logger::Error("Current working directory: %s", currentPath.c_str());
        } else {
            Logger::Error("Could not get current working directory: %s", pathEc.message().c_str());
        }
        return nullptr;
    }
    
    Logger::Info("File exists, proceeding with Assimp import...");
    
    Assimp::Importer importer;
    
    unsigned int flags = 
        aiProcess_Triangulate |
        aiProcess_JoinIdenticalVertices |
        aiProcess_GenNormals |
        aiProcess_OptimizeMeshes |
        aiProcess_ValidateDataStructure;
    
    if (flipUV) {
        flags |= aiProcess_FlipUVs;
    }
    
    if (generateTangents) {
        flags |= aiProcess_CalcTangentSpace;
    }
    
    const aiScene* scene = importer.ReadFile(filepath, flags);
    
    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        Logger::Error("Assimp Error: %s", importer.GetErrorString());
        return nullptr;
    }
    
    auto model = std::make_shared<Model>();
    model->filename = std::filesystem::path(filepath).filename().string();
    model->directory = std::filesystem::path(filepath).parent_path().string();
    
    Logger::Info("Processing %u meshes, %u materials", scene->mNumMeshes, scene->mNumMaterials);
    
    // Process materials first
    model->materials.reserve(scene->mNumMaterials);
    for (uint32_t i = 0; i < scene->mNumMaterials; ++i) {
        model->materials.push_back(ProcessMaterial(scene, scene->mMaterials[i], model->directory, device));
    }
    
    // Process node hierarchy to extract mesh data
    ProcessNode(scene, scene->mRootNode, model.get(), device, model->directory);
    
    // Calculate basic bounds
    model->CalculateBounds();
    
    Logger::Info("Model loaded successfully: %u meshes", (unsigned int)model->meshes.size());
    
    return model;
}

void ModelLoader::ProcessNode(
    const aiScene* scene,
    aiNode* node,
    Model* model,
    std::shared_ptr<Device> device,
    const std::string& directory) {
    
    // Process all meshes in this node
    for (uint32_t i = 0; i < node->mNumMeshes; ++i) {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        auto processedMesh = ProcessMesh(scene, mesh, mesh->mMaterialIndex, device);
        if (processedMesh) {
            model->meshes.push_back(processedMesh);
        }
    }
    
    // Process children recursively
    for (uint32_t i = 0; i < node->mNumChildren; ++i) {
        ProcessNode(scene, node->mChildren[i], model, device, directory);
    }
}

// Helper functions for Assimp type conversion
namespace {
    Vector3 AssimpToVector3(const aiVector3D& vec) {
        return Vector3(vec.x, vec.y, vec.z);
    }

    Vector2 AssimpToVector2(const aiVector3D& vec) {
        return Vector2(vec.x, vec.y);
    }
}

std::shared_ptr<Mesh> ModelLoader::ProcessMesh(
    const aiScene* scene,
    aiMesh* mesh,
    uint32_t materialIndex,
    std::shared_ptr<Device> device) {
    
    std::vector<ModelVertex> vertices;
    std::vector<uint32_t> indices;
    
    // Process vertices
    vertices.reserve(mesh->mNumVertices);
    for (uint32_t i = 0; i < mesh->mNumVertices; ++i) {
        ModelVertex vertex{};
        
        // Position
        vertex.position = AssimpToVector3(mesh->mVertices[i]);
        
        // Normal
        if (mesh->mNormals) {
            vertex.normal = AssimpToVector3(mesh->mNormals[i]);
        } else {
            vertex.normal = Vector3(0.0f, 1.0f, 0.0f); // Default normal
        }
        
        // Texture coordinates (use first set)
        if (mesh->mTextureCoords[0]) {
            vertex.texcoord = Vector2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y);
        } else {
            vertex.texcoord = Vector2(0.0f, 0.0f); // Default UV
        }
        
        // Tangent and bitangent
        if (mesh->mTangents) {
            vertex.tangent = AssimpToVector3(mesh->mTangents[i]);
        }
        if (mesh->mBitangents) {
            vertex.bitangent = AssimpToVector3(mesh->mBitangents[i]);
        }
        
        vertices.push_back(vertex);
    }
    
    // Process indices
    for (uint32_t i = 0; i < mesh->mNumFaces; ++i) {
        aiFace face = mesh->mFaces[i];
        for (uint32_t j = 0; j < face.mNumIndices; ++j) {
            indices.push_back(face.mIndices[j]);
        }
    }
    
    auto athenaMesh = std::make_shared<Mesh>();
    athenaMesh->SetMaterialIndex(materialIndex);
    
    // Store vertex and index data for later use
    athenaMesh->SetVertexData(vertices.data(), vertices.size() * sizeof(ModelVertex), sizeof(ModelVertex));
    athenaMesh->SetIndexData(indices.data(), indices.size() * sizeof(uint32_t), indices.size());
    
    // Create GPU buffers immediately
    athenaMesh->CreateBuffers(device->GetD3D12Device());
    
    Logger::Info("Processed mesh with %u vertices, %u indices", (unsigned int)vertices.size(), (unsigned int)indices.size());
    
    return athenaMesh;
}

MaterialData ModelLoader::ProcessMaterial(
    const aiScene* scene,
    aiMaterial* mat,
    const std::string& directory,
    std::shared_ptr<Device> device) {
    
    MaterialData material;
    material.name = "default";
    return material;
}

std::string ModelLoader::GetTexturePathFromMaterial(
    aiMaterial* mat,
    aiTextureType type,
    const std::string& directory) {
    
    return "";
}

std::shared_ptr<Texture> ModelLoader::LoadTexture(
    const std::string& path,
    const std::string& directory,
    std::shared_ptr<Device> device,
    bool sRGB) {
    
    return nullptr;
}


std::vector<std::string> ModelLoader::GetSupportedExtensions() {
    return {
        ".obj", ".fbx", ".dae", ".gltf", ".glb", 
        ".3ds", ".blend", ".x", ".md5", ".ply",
        ".stl", ".ase", ".ifc", ".dxf"
    };
}

bool ModelLoader::IsExtensionSupported(const std::string& extension) {
    auto extensions = GetSupportedExtensions();
    return std::find(extensions.begin(), extensions.end(), extension) != extensions.end();
}

void Model::CalculateBounds() {
    minBounds = Vector3(-1, -1, -1);
    maxBounds = Vector3(1, 1, 1);
}

} // namespace Athena