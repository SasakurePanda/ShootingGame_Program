//#include "ModelComponent.h"
//
//#include "GameObject.h"
//#include <iostream>
//
//bool ModelComponent::LoadModel(const std::string& filepath)
//{
//    m_model = std::make_unique<Model>();
//
//    if (!m_model->LoadFromFile(filepath))
//    {
//        std::cout << "ModelComponent: モデルの読み込みに失敗しました：" << filepath << "\n";
//        return false;
//    }
//
//    return true;
//}
//
//void ModelComponent::Draw()
//{
//    if (!m_model || !m_owner) return;
//
//    std::cout << "[ModelComponent] Draw呼ばれた\n";
//
//    const SRT& srt = m_owner->GetTransform();
//    const Matrix4x4 worldMatrix = srt.GetMatrix();
//
//    m_model->Draw(worldMatrix);
//}
#include "ModelComponent.h"
#include "Renderer.h"
#include "Application.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <WICTextureLoader.h> // テクスチャ読み込み用
#include <filesystem> 
#include "TextureManager.h"

ModelComponent::ModelComponent(const std::string& filepath)
    : m_filepath(filepath)
{
}

void ModelComponent::Initialize()
{
    LoadModel(m_filepath);
}

void ModelComponent::Update(float dt)
{
    // モデル自体に動きがない場合は空でもOK
}

void ModelComponent::Draw(float alpha)
{
    for (const auto& mesh : m_meshes)
    {
        // 頂点バッファ設定
        UINT stride = sizeof(VERTEX_3D);
        UINT offset = 0;
        Renderer::GetDeviceContext()->IASetVertexBuffers(0, 1, mesh.vertexBuffer.GetAddressOf(), &stride, &offset);

        // インデックスバッファ設定
        Renderer::GetDeviceContext()->IASetIndexBuffer(mesh.indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);

        // プリミティブタイプ設定（例：三角形）
        Renderer::GetDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        Matrix4x4 worldMatrix = GetOwner()->GetTransform().GetMatrix();
        Renderer::SetWorldMatrix(&worldMatrix);


        // マテリアル・テクスチャ設定（仮）
        ID3D11ShaderResourceView* nullSRV[1] = { nullptr };
        if (mesh.texture)
        {
            Renderer::SetTexture(mesh.texture.Get());
        }
        else
        {
            Renderer::GetDeviceContext()->PSSetShaderResources(0, 1, nullSRV); // 明示的にクリア
        }
            


        // 描画
        Renderer::GetDeviceContext()->DrawIndexed(mesh.indexCount, 0, 0);


        Renderer::GetDeviceContext()->PSSetShaderResources(0, 1, nullSRV);
    }
}



void ModelComponent::LoadModel(const std::string& path)
{
    // ----------------------------
    // 親ディレクトリ作成
    // ----------------------------
    {
        std::filesystem::path p(path);
        m_directory = p.parent_path().string();
    }

    // --- デバッグ出力（読み込もうとしている情報） ---
    {
        std::string msg;
        msg += "LoadModel called\n";
        msg += "  path: " + path + "\n";
        msg += "  m_directory: " + m_directory + "\n";
        msg += std::string("  file exists: ") + (std::filesystem::exists(path) ? "yes" : "no") + "\n";
        OutputDebugStringA(msg.c_str());
        // または std::cout << msg;
    }

    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(
        path,
        aiProcess_Triangulate /* | aiProcess_FlipUVs */ | aiProcess_CalcTangentSpace
    );

    if (!scene || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) || !scene->mRootNode)
    {
        // Assimpのエラー文字列を出しておくと原因がわかりやすい
        const char* err = importer.GetErrorString();
        char buf[1024];
        sprintf_s(buf, "Assimp ReadFile failed. scene=%p flags=0x%08X err=%s\n",
            scene, (unsigned)importer.GetPropertyInteger(AI_CONFIG_PP_RVC_FLAGS), err ? err : "(null)");
        OutputDebugStringA(buf);

        // 追加で詳細を出したければ
        std::string more;
        more += "  attempted file: " + path + "\n";
        more += "  parent dir: " + m_directory + "\n";
        OutputDebugStringA(more.c_str());

        return;
    }

    // 成功時の一報（任意）
    {
        char ok[256];
        sprintf_s(ok, "Assimp ReadFile succeeded. numMeshes=%u numMaterials=%u\n",
            scene->mNumMeshes, scene->mNumMaterials);
        OutputDebugStringA(ok);
    }

    ProcessNode(scene->mRootNode, scene);
}

void ModelComponent::ProcessNode(aiNode* node, const aiScene* scene)
{
    // このノードに含まれるすべてのメッシュを処理
    for (UINT i = 0; i < node->mNumMeshes; ++i)
    {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        ProcessMesh(mesh, scene);
    }

    // 子ノードを再帰的に処理
    for (UINT i = 0; i < node->mNumChildren; ++i)
    {
        ProcessNode(node->mChildren[i], scene);
    }

    int i = 0;
}


void ModelComponent::ProcessMesh(aiMesh* mesh, const aiScene* scene)
{
    std::vector<VERTEX_3D> vertices;
    std::vector<uint32_t> indices;

    // 頂点データの構築
    for (unsigned int i = 0; i < mesh->mNumVertices; ++i)
    {
        VERTEX_3D vertex = {};

        // 位置
        vertex.Position = {
            mesh->mVertices[i].x,
            mesh->mVertices[i].y,
            mesh->mVertices[i].z
        };

        // 法線
        if (mesh->HasNormals())
        {
            vertex.Normal = {
                mesh->mNormals[i].x,
                mesh->mNormals[i].y,
                mesh->mNormals[i].z
            };
        }

        // テクスチャ座標（UV）
        if (mesh->HasTextureCoords(0))
        {
            vertex.TexCoord = {
                mesh->mTextureCoords[0][i].x,
                mesh->mTextureCoords[0][i].y
            };
        }

        // 拡散色（Assimpでは頂点カラーはオプション）
        vertex.Diffuse = Color(1.0f, 1.0f, 1.0f, 1.0f); // 白で初期化

        // ボーン情報は後で埋める（Assimpのボーン処理が必要）

        vertices.push_back(vertex);
    }

    // インデックスデータの構築
    for (unsigned int i = 0; i < mesh->mNumFaces; ++i)
    {
        const aiFace& face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; ++j)
        {
            indices.push_back(face.mIndices[j]);
        }
    }

    // ボーン情報の埋め込み（最大4つまで）
    if (mesh->HasBones())
    {
        for (unsigned int b = 0; b < mesh->mNumBones; ++b)
        {
            const aiBone* bone = mesh->mBones[b];
            for (unsigned int w = 0; w < bone->mNumWeights; ++w)
            {
                const aiVertexWeight& weight = bone->mWeights[w];
                int vIndex = weight.mVertexId;
                float wValue = weight.mWeight;

                for (int i = 0; i < 4; ++i)
                {
                    if (vertices[vIndex].BoneWeight[i] == 0.0f)
                    {
                        vertices[vIndex].BoneIndex[i] = b;
                        vertices[vIndex].BoneWeight[i] = wValue;
                        break;
                    }
                }
            }
        }
    }

    // マテリアル取得
    aiMaterial* material = nullptr;
    if (mesh->mMaterialIndex >= 0)
    {
        material = scene->mMaterials[mesh->mMaterialIndex];
    }

    // MeshData に格納
    MeshData meshData = {};

    // テクスチャ読み込み
    if (material)
    {
        aiString path;
        if (material->GetTexture(aiTextureType_DIFFUSE, 0, &path) == AI_SUCCESS)
        {
            std::string texturePath = m_directory + "/" + path.C_Str();
            meshData.texture = TextureManager::Load(texturePath);
        }
    }

    // 頂点バッファ作成
    D3D11_BUFFER_DESC vbDesc = {};
    vbDesc.Usage = D3D11_USAGE_DEFAULT;
    vbDesc.ByteWidth = sizeof(VERTEX_3D) * static_cast<UINT>(vertices.size());
    vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vbDesc.CPUAccessFlags = 0;

    D3D11_SUBRESOURCE_DATA vbData = {};
    vbData.pSysMem = vertices.data();

    ComPtr<ID3D11Buffer> vertexBuffer;
    Renderer::GetDevice()->CreateBuffer(&vbDesc, &vbData, vertexBuffer.GetAddressOf());

    // インデックスバッファ作成
    D3D11_BUFFER_DESC ibDesc = {};
    ibDesc.Usage = D3D11_USAGE_DEFAULT;
    ibDesc.ByteWidth = sizeof(uint32_t) * static_cast<UINT>(indices.size());
    ibDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    ibDesc.CPUAccessFlags = 0;

    D3D11_SUBRESOURCE_DATA ibData = {};
    ibData.pSysMem = indices.data();

    ComPtr<ID3D11Buffer> indexBuffer;
    Renderer::GetDevice()->CreateBuffer(&ibDesc, &ibData, indexBuffer.GetAddressOf());

    meshData.vertexBuffer = vertexBuffer;
    meshData.indexBuffer = indexBuffer;
    meshData.indexCount = static_cast<UINT>(indices.size());

    m_meshes.push_back(meshData);
}