#include "Primitive.h"

void Primitive::CreateSphere(ID3D11Device* device, float radius, int sliceCount, int stackCount)
{
    vertices.clear();   //頂点データを削除
    indices.clear();    //インデックスバッファを削除

    // 頂点の生成
    for (int i = 0; i <= stackCount; ++i)       //縦数の分割数分だけ回す
    {
        float phi = XM_PI * i / stackCount;
        float y = radius * cosf(phi);
        float r = radius * sinf(phi);

        for (int j = 0; j <= sliceCount; ++j)   //横の分割数分だけ回す
        {
            float theta = XM_2PI * j / sliceCount;
            float x = r * cosf(theta);
            float z = r * sinf(theta);

            vertices.push_back({ XMFLOAT3(x, y, z), XMFLOAT3(x, y, z), XMFLOAT2((float)j / sliceCount, (float)i / stackCount) });
        }
    }

    // インデックスの生成
    for (int i = 0; i < stackCount; ++i)
    {
        for (int j = 0; j < sliceCount; ++j)
        {
            indices.push_back(i * (sliceCount + 1) + j);
            indices.push_back((i + 1) * (sliceCount + 1) + j);
            indices.push_back((i + 1) * (sliceCount + 1) + j + 1);

            indices.push_back(i * (sliceCount + 1) + j);
            indices.push_back((i + 1) * (sliceCount + 1) + j + 1);
            indices.push_back(i * (sliceCount + 1) + j + 1);
        }
    }

    CreateBuffers(device);
}

void Primitive::CreateBox(ID3D11Device* device, float width, float height, float depth)
{
    vertices.clear();
    indices.clear();

    float w2 = width * 0.5f;
    float h2 = height * 0.5f;
    float d2 = depth * 0.5f;

    // 8頂点
    XMFLOAT3 positions[] =
    {
        { -w2, -h2, -d2 }, { w2, -h2, -d2 }, { w2, h2, -d2 }, { -w2, h2, -d2 },
        { -w2, -h2, d2 }, { w2, -h2, d2 }, { w2, h2, d2 }, { -w2, h2, d2 }
    };

    // インデックス
    uint32_t boxIndices[] =
    {
        0,1,2,  2,3,0, // 前面
        4,5,6,  6,7,4, // 背面
        0,4,7,  7,3,0, // 左面
        1,5,6,  6,2,1, // 右面
        3,2,6,  6,7,3, // 上面
        0,1,5,  5,4,0  // 底面
    };

    for (int i = 0; i < 8; ++i)
    {
        vertices.push_back({ positions[i], XMFLOAT3(0, 0, -1), XMFLOAT2(0, 0) });
    }

    indices.assign(boxIndices, boxIndices + sizeof(boxIndices) / sizeof(uint32_t));

    CreateBuffers(device);
}

void Primitive::CreateBuffers(ID3D11Device* device)
{
    // 頂点バッファ作成
    D3D11_BUFFER_DESC bufferDesc = {};
    bufferDesc.Usage = D3D11_USAGE_DEFAULT;
    bufferDesc.ByteWidth = sizeof(Vertex) * vertices.size();
    bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = vertices.data();

    device->CreateBuffer(&bufferDesc, &initData, &vertexBuffer);

    // インデックスバッファ作成
    bufferDesc.ByteWidth = sizeof(uint32_t) * indices.size();
    bufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    initData.pSysMem = indices.data();

    device->CreateBuffer(&bufferDesc, &initData, &indexBuffer);
}

void Primitive::Draw(ID3D11DeviceContext* context)
{
    UINT stride = sizeof(Vertex);
    UINT offset = 0;
    context->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
    context->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R32_UINT, 0);
    context->DrawIndexed(indices.size(), 0, 0);
}
