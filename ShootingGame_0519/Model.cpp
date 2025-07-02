#include "Model.h"
#include <WICTextureLoader.h> 
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include "renderer.h"
#include <filesystem>
#include <iostream>
//------------------------------------------------------------
//モデルのファイルを読み込む関数(引数：モデルファイルがある所のパス)
//------------------------------------------------------------
bool Model::LoadFromFile(const std::string& path)
{
    //ファイルパスからディレクトリ部分を抜き出して保持
    size_t pos = path.find_last_of("/\\");

    if (pos != std::string::npos)
    {
        modelDirectory_ = path.substr(0, pos + 1);
    }
    else
    {
        modelDirectory_.clear();
    }

    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(path,
        aiProcess_Triangulate |
        aiProcess_GenSmoothNormals |
        aiProcess_FlipUVs |
        aiProcess_FlipWindingOrder   // ← これを追加
    );
    if (!scene || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) || !scene->mRootNode) 
    {
        return false;
    }

    ProcessNode(scene->mRootNode, scene);
    return true;
}

void Model::ProcessNode(aiNode* node, const aiScene* scene)
{
    // ノード内の各メッシュを処理
    for (UINT i = 0; i < node->mNumMeshes; ++i)
    {
        aiMesh* am = scene->mMeshes[node->mMeshes[i]];
        meshes_.push_back(ProcessMesh(am, scene));
    }
    // 子ノードも再帰
    for (UINT i = 0; i < node->mNumChildren; ++i)
    {
        ProcessNode(node->mChildren[i], scene);
    }
}

MeshPart Model::ProcessMesh(aiMesh* mesh, const aiScene* scene)
{
    aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

    // —— ここからログ出力コード ——
    std::vector<aiTextureType> types =
    {
        aiTextureType_DIFFUSE,
        aiTextureType_AMBIENT,
        aiTextureType_EMISSIVE,
        aiTextureType_BASE_COLOR, // PBR用
        aiTextureType_SPECULAR,
        aiTextureType_NORMALS
    };

    for (auto t : types)
    {
        unsigned count = material->GetTextureCount(t);
        if (count > 0)
        {
            aiString path;
            material->GetTexture(t, 0, &path);
            std::cout << "TextureType " << t
                << " count=" << count
                << " path=" << path.C_Str() << "\n";
        }
        else
        {
            std::cout << "TextureType " << t << " count=0\n";
        }
    }

    std::vector<VERTEX_3D> verts(mesh->mNumVertices);
    std::vector<UINT> indices;
    indices.reserve(mesh->mNumFaces * 3);

    // マテリアル取得
    //aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
    aiColor4D c;
    material->Get(AI_MATKEY_COLOR_DIFFUSE, c);
    Vector4 diffuseColor(c.r, c.g, c.b, c.a);

    // テクスチャパス取得
    aiString texPath;
    bool hasTexture = false;
    if (material->GetTextureCount(aiTextureType_DIFFUSE) > 0 && material->GetTexture(aiTextureType_DIFFUSE, 0, &texPath) == AI_SUCCESS)
    {
        hasTexture = true;
    }
    // PBR対応：BaseColorテクスチャ
    else if (material->GetTextureCount(aiTextureType_BASE_COLOR) > 0 && material->GetTexture(aiTextureType_BASE_COLOR, 0, &texPath) == AI_SUCCESS)
    {
        hasTexture = true;
    }

    // 頂点情報転送
    for (UINT i = 0; i < mesh->mNumVertices; ++i) 
    {
        verts[i].Position = { mesh->mVertices[i].x,
                              mesh->mVertices[i].y,
                              mesh->mVertices[i].z };
        verts[i].Normal = { mesh->mNormals[i].x,
                            mesh->mNormals[i].y,
                            mesh->mNormals[i].z };
        if (mesh->mTextureCoords[0]) {
            verts[i].TexCoord = { mesh->mTextureCoords[0][i].x,
                                  mesh->mTextureCoords[0][i].y };
        }
        else 
        {
            verts[i].TexCoord = { 0,0 };
        }
        // マテリアルのDiffuse色を設定
        verts[i].Diffuse = diffuseColor;
    }

    // インデックス
    for (UINT f = 0; f < mesh->mNumFaces; ++f) 
    {
        const aiFace& face = mesh->mFaces[f];
        indices.insert(indices.end(), face.mIndices, face.mIndices + 3);
    }

    MeshPart part;
    auto device = Renderer::GetDevice();
    D3D11_BUFFER_DESC bd{};
    D3D11_SUBRESOURCE_DATA sd{};

    // 頂点バッファ
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bd.ByteWidth = UINT(sizeof(VERTEX_3D) * verts.size());
    sd.pSysMem = verts.data();
    device->CreateBuffer(&bd, &sd, part.vb.GetAddressOf());

    // インデックスバッファ
    bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    bd.ByteWidth = UINT(sizeof(UINT) * indices.size());
    sd.pSysMem = indices.data();
    device->CreateBuffer(&bd, &sd, part.ib.GetAddressOf());
    part.indexCount = UINT(indices.size());

    // テクスチャ読み込み
    if (hasTexture)
    {
        // テクスチャパスをモデルフォルダ付きで絶対パス化
        std::string fullTexPath = modelDirectory_ + texPath.C_Str();
        if (std::filesystem::path(texPath.C_Str()).is_absolute()) 
        {
            fullTexPath = texPath.C_Str();
        }

        // ワイド文字列に変換
        int len = MultiByteToWideChar(CP_UTF8, 0, fullTexPath.c_str(), -1, nullptr, 0);
        std::wstring wpath(len, L'\0');
        MultiByteToWideChar(CP_UTF8, 0, fullTexPath.c_str(), -1, &wpath[0], len);

        // SRVを生成
        HRESULT hr = DirectX::CreateWICTextureFromFile(
            device, Renderer::GetDeviceContext(),
            wpath.c_str(), nullptr,
            part.textureSRV.ReleaseAndGetAddressOf());

        if (FAILED(hr))
        {
            OutputDebugStringA(("テクスチャ読み込み失敗: " + fullTexPath + "\n").c_str());
        }
    }
    return part;
}


void Model::Draw(const SRT& transform)
{
    Matrix4x4 world = transform.GetMatrix().Transpose();
    Renderer::GetDeviceContext()->UpdateSubresource(
        Renderer::GetWorldBuffer(), 0, nullptr, &world, 0, 0);

    auto dc = Renderer::GetDeviceContext();
    UINT stride = sizeof(VERTEX_3D), offset = 0;
    for (auto& mesh : meshes_)
    {
        dc->IASetVertexBuffers(0, 1, mesh.vb.GetAddressOf(), &stride, &offset);
        dc->IASetIndexBuffer(mesh.ib.Get(), DXGI_FORMAT_R32_UINT, 0);

        dc->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        if (mesh.textureSRV)
        {
            dc->PSSetShaderResources(0, 1, mesh.textureSRV.GetAddressOf());
        }
        dc->DrawIndexed(mesh.indexCount, 0, 0);
    }
}
