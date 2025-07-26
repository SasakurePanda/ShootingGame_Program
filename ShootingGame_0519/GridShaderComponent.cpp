#include "GridShaderComponent.h"
#include "ModelComponent.h"
#include "Renderer.h"

struct Vertex
{
    float x, y, z;
    float r, g, b, a; // 色（例えばグレー系）
};

GridShaderComponent::GridShaderComponent()
{
}

GridShaderComponent::~GridShaderComponent()
{
}

void GridShaderComponent::Initialize()
{
    CreateGridVertices();
    // Grid用シェーダーは Renderer::BindGridShader() でセットする想定
}

void GridShaderComponent::CreateGridVertices()
{
    // 例えば10x10グリッドで線間隔1.0f
    const int lineCount = 10;
    const float spacing = 1.0f;
    const float halfSize = (lineCount * spacing) / 2.0f;

    m_VertexCount = lineCount * 4; // 10本ずつX,Z軸に線 -> 20線 * 2点 = 40点

    Vertex vertices[40]; // 固定サイズで仮置き

    int idx = 0;
    for (int i = 0; i < lineCount; ++i)
    {
        float pos = -halfSize + i * spacing;

        // X方向の線 (Z = pos)
        vertices[idx++] = { -halfSize, 0.0f, pos, 0.7f, 0.7f, 0.7f, 1.0f };
        vertices[idx++] = { halfSize, 0.0f, pos, 0.7f, 0.7f, 0.7f, 1.0f };

        // Z方向の線 (X = pos)
        vertices[idx++] = { pos, 0.0f, -halfSize, 0.7f, 0.7f, 0.7f, 1.0f };
        vertices[idx++] = { pos, 0.0f,  halfSize, 0.7f, 0.7f, 0.7f, 1.0f };
    }

    // DirectXのバッファ作成
    D3D11_BUFFER_DESC bd{};
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(Vertex) * m_VertexCount;
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    D3D11_SUBRESOURCE_DATA initData{};
    initData.pSysMem = vertices;

    HRESULT hr = Renderer::GetDevice()->CreateBuffer(&bd, &initData, m_VertexBuffer.GetAddressOf());
    if (FAILED(hr))
    {
        // エラー処理
    }
}

void GridShaderComponent::Draw()
{
    auto model = GetOwner()->GetComponent<ModelComponent>();
    if (!model) return;

    // グリッド用シェーダーをバインド
    Renderer::BindGridShader();

    // モデルを描画
    model->Draw();
}
