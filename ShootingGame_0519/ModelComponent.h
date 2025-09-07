//#pragma once
//#include "Component.h"
//#include "Model.h"
//#include <memory>
//
//class ModelComponent : public Component
//{
//public:
//    ModelComponent() = default;
//    virtual ~ModelComponent() override = default;
//
//    // モデルを読み込む
//    bool LoadModel(const std::string& filepath);
//
//    // 描画（まだTransform連携していないので仮引数）
//    void Draw() override;
//
//private:
//    std::unique_ptr<Model> m_model;
//};

#pragma once
#include "Component.h"
#include "renderer.h"
#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <string>
#include <vector>
#include <memory>
#include <d3d11.h>
#include <wrl/client.h>

class ModelComponent : public Component
{
public:
    ModelComponent() {}
    ModelComponent(const std::string& filepath);
    ~ModelComponent() override = default;
    void LoadModel(const std::string& filepath);
    void Initialize() override;
    void Update(float dt) override;
    void Draw(float alpha) override;

private:
 
    void ProcessNode(aiNode* node, const aiScene* scene);
    void ProcessMesh(aiMesh* mesh, const aiScene* scene);

    struct MeshData
    {
        Microsoft::WRL::ComPtr<ID3D11Buffer> vertexBuffer;
        Microsoft::WRL::ComPtr<ID3D11Buffer> indexBuffer;
        UINT indexCount;
        MATERIAL material;
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> texture;
    };

    std::vector<MeshData> m_meshes;
    std::string m_filepath;
    std::string m_directory;
};