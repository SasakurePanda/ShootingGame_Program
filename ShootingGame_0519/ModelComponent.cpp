#include "ModelComponent.h"
#include "Renderer.h"
#include "GameObject.h"
#include "Application.h"
#include "TextureManager.h" // 既存の TextureManager を使用
#include <WICTextureLoader.h>
#include <iostream>

#ifdef _WIN32
#include <windows.h> // MultiByteToWideChar を使うため
#endif
#include <filesystem>

using Microsoft::WRL::ComPtr;

// コンストラクタ (ファイルパス指定)
ModelComponent::ModelComponent(const std::string& filepath)
    : m_filepath(filepath)
{
}

// Initialize() で読み込む
void ModelComponent::Initialize()
{
    if (!m_filepath.empty())
    {
        LoadModel(m_filepath);
    }
}

// Update は後でアニメーションをここで更新する想定
void ModelComponent::Update(float dt)
{
    // 将来: アニメーションのタイムラインを進めてボーン行列を計算し、
    // 結果をシェーダへ転送する処理をここに書く
}

// 描画 (簡易)
void ModelComponent::Draw(float alpha)
{
    // ワールド行列設定
    Matrix4x4 worldMatrix = GetOwner()->GetTransform().GetMatrix();
    Renderer::SetWorldMatrix(&worldMatrix);

    // メッシュごとに描画
    for (auto& mesh : m_meshes)
    {
        // まず色をセット (ここで Diffuse 色をピクセルシェーダに渡す)
        ID3D11DeviceContext* ctx = Renderer::GetDeviceContext();
        // cbMaterial の構造体が Renderer 内にある想定
        MATERIAL cb;
        cb.Diffuse = DirectX::XMFLOAT4(
            mesh.material.Diffuse.x,
            mesh.material.Diffuse.y,
            mesh.material.Diffuse.z,
            mesh.material.Diffuse.w
        );
        ctx->UpdateSubresource(Renderer::GetMaterialCB(), 0, nullptr, &cb, 0, 0);
        ctx->PSSetConstantBuffers(0, 1, Renderer::GetMaterialCBAddress());

        // 以下、既存の頂点/インデックス/テクスチャ設定
        UINT stride = sizeof(VERTEX_3D);
        UINT offset = 0;
        ctx->IASetVertexBuffers(0, 1, mesh.vertexBuffer.GetAddressOf(), &stride, &offset);
        ctx->IASetIndexBuffer(mesh.indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
        ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        if (mesh.srvDiffuse)
        {
            ctx->PSSetShaderResources(0, 1, mesh.srvDiffuse.GetAddressOf());
        }

        ctx->DrawIndexed(mesh.indexCount, 0, 0);
    }
}


void ModelComponent::SetColor(const Color& color)
{
    // シンプルに全メッシュのマテリアル Diffuse を上書き
    for (auto& mesh : m_meshes)
    {
        mesh.material.Diffuse = color;
    }
}

// ファイル存在チェック
bool ModelComponent::FileExists(const std::string& path)
{
    try {
#ifdef _WIN32
        // UTF-8 -> wstring に変換して path を作る（Windows はワイド文字を扱う）
        int len = ::MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1, nullptr, 0);
        if (len <= 0) return false;
        std::wstring wpath(len, L'\0');
        ::MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1, &wpath[0], len);
        std::filesystem::path p(wpath);
#else
        // POSIX 系なら std::string をそのまま使う
        std::filesystem::path p(path);
#endif
        return std::filesystem::exists(p);
    }
    catch (...) {
        return false;
    }
}


// マテリアルとテクスチャの読み込み
void ModelComponent::LoadMaterials(const aiScene* scene)
{
    m_materials.clear();
    m_materials.resize(scene->mNumMaterials);

    for (UINT i = 0; i < scene->mNumMaterials; ++i)
    {
        aiMaterial* aimat = scene->mMaterials[i];
        MATERIAL mat{}; // 既存の MATERIAL 構造体に合わせて初期化

        // 名前
        aiString name;
        if (AI_SUCCESS == aimat->Get(AI_MATKEY_NAME, name))
        {
            // MATERIAL に名前フィールドがあればセット (ここは仮)
            // mat.Name = name.C_Str();
        }

        // カラー (Diffuse/Specular/Ambient など)
        aiColor4D col;
        if (AI_SUCCESS == aimat->Get(AI_MATKEY_COLOR_DIFFUSE, col))
        {
            mat.Diffuse = Color(col.r, col.g, col.b, col.a);
        }
        if (AI_SUCCESS == aimat->Get(AI_MATKEY_COLOR_AMBIENT, col))
        {
            mat.Ambient = Color(col.r, col.g, col.b, col.a);
        }
        if (AI_SUCCESS == aimat->Get(AI_MATKEY_COLOR_SPECULAR, col))
        {
            mat.Specular = Color(col.r, col.g, col.b, col.a);
        }

        // テクスチャは MeshData 側で個別に取得するが、
        // ここでパスだけ取りたい場合は取得しておくことも可能。

        m_materials[i] = mat;
    }
}

// aiMaterial から特定のテクスチャタイプを読み込んで SRV を返す (存在しない場合は nullptr)
Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> ModelComponent::LoadTextureFromMaterial(aiMaterial* mat, aiTextureType t)
{
    aiString texPath;
    if (mat->GetTextureCount(t) > 0 && mat->GetTexture(t, 0, &texPath) == AI_SUCCESS)
    {
        std::string tex = texPath.C_Str();

        // 埋め込みテクスチャなら (例: "*0") ここで検出
        if (!tex.empty() && tex[0] == '*')
        {
            return nullptr;
        }

        // ファイルパスの正規化 (絶対/相対の判定)
        std::filesystem::path p(tex);
        if (!p.is_absolute())
        {
            p = std::filesystem::path(m_directory) / p;
        }

        std::string fullPath = p.string();

        // 既存の TextureManager を使ってロード (UTF8 -> 変換は内部でやられている想定)
        ID3D11ShaderResourceView* srv = TextureManager::Load(fullPath);

        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> comsrv = srv;
        return comsrv;
    }
    return nullptr;
}

// 実際のモデル読み込み処理
void ModelComponent::LoadModel(const std::string& path)
{
    // ディレクトリ部分を取得して保持 (テクスチャ読み込みに使用)
    {
        std::filesystem::path p(path);
        m_directory = p.parent_path().string(); // std::string に安全に入る
    }

    // Assimp で読み込む (将来の為に必要なフラグを追加)
    // aiProcess_GenSmoothNormals: 法線無ければ生成
    // aiProcess_CalcTangentSpace: タンジェント空間を計算 (ノーマルマップ利用時に必要)
    unsigned int flags =
        aiProcess_Triangulate |
        aiProcess_GenSmoothNormals |
        aiProcess_CalcTangentSpace |
        aiProcess_JoinIdenticalVertices |
        aiProcess_SortByPType;

    // m_importer はメンバなのでここで読み込んで m_scene に保持する
    m_scene = m_importer.ReadFile(path, flags);

    if (!m_scene || (m_scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) || !m_scene->mRootNode)
    {
        const char* err = m_importer.GetErrorString();
        std::string s = "Assimp ReadFile failed: ";
        s += err ? err : "(null)";
        s += "\n";
        OutputDebugStringA(s.c_str());
        return;
    }

    // マテリアル情報の読み込み (シーン全体)
    LoadMaterials(m_scene);

    // ボーン情報収集の初期化 (ここではメッシュ処理中に追加していく)
    m_boneInfos.clear();
    m_boneNameToIndex.clear();

    // ノード再帰処理でメッシュを生成
    ProcessNode(m_scene->mRootNode, m_scene);

    // 読み込み後のログ
    {
        char buf[256];
        sprintf_s(buf, "Model loaded: meshes=%zu bones=%zu materials=%zu\n",
            m_meshes.size(), m_boneInfos.size(), m_materials.size());
        OutputDebugStringA(buf);
    }

    // (将来) ここでボーン用の定数バッファを作成することを推奨
    // 例: m_cbBones = CreateConstantBuffer(sizeof(XMMATRIX) * MAX_BONES);
}

// ノード再帰
void ModelComponent::ProcessNode(aiNode* node, const aiScene* scene)
{
    // ノード内の全メッシュを処理
    for (UINT i = 0; i < node->mNumMeshes; ++i)
    {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        ProcessMesh(mesh, scene);
    }

    // 子ノードを再帰
    for (UINT i = 0; i < node->mNumChildren; ++i)
    {
        ProcessNode(node->mChildren[i], scene);
    }
}

// メッシュ処理
void ModelComponent::ProcessMesh(aiMesh* mesh, const aiScene* scene)
{
    // ローカルに頂点・インデックスを作る
    std::vector<VERTEX_3D> vertices;
    vertices.resize(mesh->mNumVertices);

    // インデックス配列 (faces は三角形化されている前提)
    std::vector<uint32_t> indices;
    indices.reserve(mesh->mNumFaces * 3);

    // まず、頂点情報を埋める
    for (unsigned int i = 0; i < mesh->mNumVertices; ++i)
    {
        VERTEX_3D v{};
        // 位置
        v.Position = { mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z };

        // 法線 (生成済みのはず)
        if (mesh->HasNormals())
        {
            v.Normal = { mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z };
        }
        else
        {
            v.Normal = { 0.f, 1.f, 0.f }; // フォールバック
        }

        // UV
        if (mesh->HasTextureCoords(0))
        {
            v.TexCoord = { mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y };
        }
        else
        {
            v.TexCoord = { 0.f, 0.f };
        }

        // 頂点色 (もしあれば)
        if (mesh->HasVertexColors(0))
        {
            aiColor4D c = mesh->mColors[0][i];
            v.Diffuse = Color(c.r, c.g, c.b, c.a);
        }
        else
        {
            v.Diffuse = Color(1, 1, 1, 1);
        }

        // ボーン関連を初期化 (最大4ウェイト想定)
        for (int bi = 0; bi < 4; ++bi)
        {
            v.BoneIndex[bi] = 0;
            v.BoneWeight[bi] = 0.0f;
        }
        v.bonecnt = 0;

        vertices[i] = v;
    }

    // インデックス埋め
    for (unsigned int f = 0; f < mesh->mNumFaces; ++f)
    {
        const aiFace& face = mesh->mFaces[f];
        // face.mNumIndices は 3 のはず (triangulate)
        for (unsigned int k = 0; k < face.mNumIndices; ++k)
        {
            indices.push_back(face.mIndices[k]);
        }
    }

    // ボーン情報の収集・頂点への反映
    if (mesh->HasBones())
    {
        for (unsigned int b = 0; b < mesh->mNumBones; ++b)
        {
            aiBone* aibone = mesh->mBones[b];
            std::string boneName = aibone->mName.C_Str();

            int boneIndex = -1;
            auto it = m_boneNameToIndex.find(boneName);
            if (it == m_boneNameToIndex.end())
            {
                // 新しいボーンなら追加してインデックスを振る
                BoneInfo bi;
                bi.name = boneName;
                bi.offsetMatrix = aibone->mOffsetMatrix;
                bi.finalTransform = aiMatrix4x4(); // 初期化
                boneIndex = static_cast<int>(m_boneInfos.size());
                m_boneInfos.push_back(bi);
                m_boneNameToIndex[boneName] = boneIndex;
            }
            else
            {
                boneIndex = it->second;
            }

            // 各頂点ウェイトの反映 (最大4つ)
            for (unsigned int w = 0; w < aibone->mNumWeights; ++w)
            {
                unsigned int vertexId = aibone->mWeights[w].mVertexId;
                float weight = aibone->mWeights[w].mWeight;

                // 頂点の BoneWeight 配列に挿入 (空きスロットを探す)
                bool placed = false;
                for (int slot = 0; slot < 4; ++slot)
                {
                    if (vertices[vertexId].BoneWeight[slot] == 0.0f)
                    {
                        vertices[vertexId].BoneIndex[slot] = boneIndex;
                        vertices[vertexId].BoneWeight[slot] = weight;
                        vertices[vertexId].bonecnt = std::max<int>(vertices[vertexId].bonecnt, slot + 1);
                        placed = true;
                        break;
                    }
                }
                if (!placed)
                {
                    // 既に4つ埋まっていたら最も小さいウェイトを置き換える簡易戦略
                    int minIdx = 0;
                    float minW = vertices[vertexId].BoneWeight[0];
                    for (int s = 1; s < 4; ++s)
                    {
                        if (vertices[vertexId].BoneWeight[s] < minW)
                        {
                            minW = vertices[vertexId].BoneWeight[s];
                            minIdx = s;
                        }
                    }
                    vertices[vertexId].BoneIndex[minIdx] = boneIndex;
                    vertices[vertexId].BoneWeight[minIdx] = weight;
                }
            }
        }

        // (オプション) 各頂点のウェイト合計が 1.0 になるよう正規化
        for (auto& v : vertices)
        {
            float sum = v.BoneWeight[0] + v.BoneWeight[1] + v.BoneWeight[2] + v.BoneWeight[3];
            if (sum > 0.0f && sum != 1.0f)
            {
                v.BoneWeight[0] /= sum;
                v.BoneWeight[1] /= sum;
                v.BoneWeight[2] /= sum;
                v.BoneWeight[3] /= sum;
            }
        }
    }

    // マテリアル取得 (mesh->mMaterialIndex が有効なら m_materials から参照)
    MATERIAL mat{};
    if (mesh->mMaterialIndex >= 0 && mesh->mMaterialIndex < (int)m_materials.size())
    {
        mat = m_materials[mesh->mMaterialIndex];
    }

    // MeshData 準備
    MeshData meshData;
    meshData.material = mat;
    meshData.indexCount = static_cast<UINT>(indices.size());

    // マテリアルからテクスチャ (Diffuse/Normal/Specular) をロード (存在すれば)
    if (mesh->mMaterialIndex >= 0)
    {
        aiMaterial* aimat = scene->mMaterials[mesh->mMaterialIndex];
        meshData.srvDiffuse = LoadTextureFromMaterial(aimat, aiTextureType_DIFFUSE);
        meshData.srvNormal = LoadTextureFromMaterial(aimat, aiTextureType_NORMALS); // Normal map
        meshData.srvSpecular = LoadTextureFromMaterial(aimat, aiTextureType_SPECULAR);
    }

    // 頂点バッファ作成
    D3D11_BUFFER_DESC vbDesc{};
    vbDesc.Usage = D3D11_USAGE_DEFAULT;
    vbDesc.ByteWidth = static_cast<UINT>(sizeof(VERTEX_3D) * vertices.size());
    vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vbDesc.CPUAccessFlags = 0;

    D3D11_SUBRESOURCE_DATA vbData{};
    vbData.pSysMem = vertices.data();

    HRESULT hr = Renderer::GetDevice()->CreateBuffer(&vbDesc, &vbData, meshData.vertexBuffer.GetAddressOf());
    if (FAILED(hr) || !meshData.vertexBuffer)
    {
        OutputDebugStringA("Failed to create vertex buffer for mesh\n");
        return;
    }

    // インデックスバッファ作成
    D3D11_BUFFER_DESC ibDesc{};
    ibDesc.Usage = D3D11_USAGE_DEFAULT;
    ibDesc.ByteWidth = static_cast<UINT>(sizeof(uint32_t) * indices.size());
    ibDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    ibDesc.CPUAccessFlags = 0;

    D3D11_SUBRESOURCE_DATA ibData{};
    ibData.pSysMem = indices.data();

    hr = Renderer::GetDevice()->CreateBuffer(&ibDesc, &ibData, meshData.indexBuffer.GetAddressOf());
    if (FAILED(hr) || !meshData.indexBuffer)
    {
        OutputDebugStringA("Failed to create index buffer for mesh\n");
        return;
    }

    // このメッシュに含まれるボーン名リスト (必要なら使う)
    if (mesh->HasBones())
    {
        meshData.boneNames.reserve(mesh->mNumBones);
        for (UINT b = 0; b < mesh->mNumBones; ++b)
        {
            meshData.boneNames.emplace_back(mesh->mBones[b]->mName.C_Str());
        }
    }
    // 最後に push_back
    m_meshes.push_back(std::move(meshData));
}
