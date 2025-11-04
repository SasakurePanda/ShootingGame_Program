#include "Primitive.h"
#include <iostream>

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

            vertices.push_back({ XMFLOAT3(x, y, z), XMFLOAT3(x, y, z), XMFLOAT4(1,1,1,1), XMFLOAT2((float)j / sliceCount, (float)i / stackCount) });
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
        vertices.push_back({ positions[i], XMFLOAT3(0,0,-1), XMFLOAT4(1,1,1,1), XMFLOAT2(0,0) });

    }

    indices.assign(boxIndices, boxIndices + sizeof(boxIndices) / sizeof(uint32_t));

    CreateBuffers(device);
}

void Primitive::CreatePlane(ID3D11Device* device, float width, float depth, int sx, int sz, const XMFLOAT4& color, bool genUV)
{
    vertices.clear();
    indices.clear();

    float widthHalf = width * 0.5f;
    float depthHalf = depth * 0.5f;

    vertices.push_back({ XMFLOAT3(-widthHalf, 0.0f, -depthHalf), XMFLOAT3(0, 1, 0), XMFLOAT4(1,1,1,1), XMFLOAT2(0.0f, 0.0f) }); // 左上
    vertices.push_back({ XMFLOAT3( widthHalf, 0.0f, -depthHalf), XMFLOAT3(0, 1, 0), XMFLOAT4(1,1,1,1), XMFLOAT2(1.0f, 0.0f) }); // 右上
    vertices.push_back({ XMFLOAT3( widthHalf, 0.0f,  depthHalf), XMFLOAT3(0, 1, 0), XMFLOAT4(1,1,1,1), XMFLOAT2(1.0f, 1.0f) }); // 右下
    vertices.push_back({ XMFLOAT3(-widthHalf, 0.0f,  depthHalf), XMFLOAT3(0, 1, 0), XMFLOAT4(1,1,1,1), XMFLOAT2(0.0f, 1.0f) }); // 左下

    //三角形をふたつ使って作成
    //インデックス設定
    uint32_t planeIndices[] =
    {
        0, 1, 2,
        0, 2, 3
    };

    indices.assign(planeIndices, planeIndices + sizeof(planeIndices) / sizeof(uint32_t));

    //GPU バッファ作成
    CreateBuffers(device);
}

void Primitive::CreateBuffers(ID3D11Device* device)
{
    // safety checks
    if (!device)
    {
        OutputDebugStringA("Primitive::CreateBuffers - device is null\n");
        return;
    }
    if (vertices.empty() || indices.empty())
    {
        char buf[256];
        sprintf_s(buf, "Primitive::CreateBuffers - empty verts=%zu idx=%zu\n", vertices.size(), indices.size());
        OutputDebugStringA(buf);
        return;
    }

    // release old buffers if any
    if (vertexBuffer) { vertexBuffer->Release(); vertexBuffer = nullptr; }
    if (indexBuffer) { indexBuffer->Release();  indexBuffer = nullptr; }

    // Vertex buffer
    D3D11_BUFFER_DESC vbDesc{};
    vbDesc.Usage = D3D11_USAGE_DEFAULT;
    vbDesc.ByteWidth = UINT(sizeof(Vertex) * vertices.size());
    vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vbDesc.CPUAccessFlags = 0;
    D3D11_SUBRESOURCE_DATA vbData{};
    vbData.pSysMem = vertices.data();
    HRESULT hr = device->CreateBuffer(&vbDesc, &vbData, &vertexBuffer);
    if (FAILED(hr) || !vertexBuffer) {
        char buf[256];
        sprintf_s(buf, "Primitive::CreateBuffers - CreateBuffer(VB) failed hr=0x%08X\n", static_cast<unsigned int>(hr));
        OutputDebugStringA(buf);
        if (vertexBuffer) { vertexBuffer->Release(); vertexBuffer = nullptr; }
        return;
    }

    // Index buffer (uint32 assumed)
    D3D11_BUFFER_DESC ibDesc{};
    ibDesc.Usage = D3D11_USAGE_DEFAULT;
    ibDesc.ByteWidth = UINT(sizeof(uint32_t) * indices.size());
    ibDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    ibDesc.CPUAccessFlags = 0;
    D3D11_SUBRESOURCE_DATA ibData{};
    ibData.pSysMem = indices.data();
    hr = device->CreateBuffer(&ibDesc, &ibData, &indexBuffer);
    if (FAILED(hr) || !indexBuffer) {
        char buf[256];
        sprintf_s(buf, "Primitive::CreateBuffers - CreateBuffer(IB) failed hr=0x%08X\n", static_cast<unsigned int>(hr));
        OutputDebugStringA(buf);
        if (indexBuffer) { indexBuffer->Release(); indexBuffer = nullptr; }
        // We keep vertexBuffer valid (or release it depending on semantics)
        return;
    }

    char buf[256];
    sprintf_s(buf, "Primitive::CreateBuffers OK verts=%zu idx=%zu vb=%p ib=%p\n",
        vertices.size(), indices.size(), vertexBuffer, indexBuffer);
    OutputDebugStringA(buf);
}


void Primitive::Draw(ID3D11DeviceContext* context)
{
    if (!context) {
        OutputDebugStringA("Primitive::Draw - context is null\n");
        return;
    }
    if (!vertexBuffer || !indexBuffer) {
        OutputDebugStringA("Primitive::Draw - missing VB or IB\n");
        return;
    }
    if (indices.empty()) {
        OutputDebugStringA("Primitive::Draw - indices empty\n");
        return;
    }

    UINT stride = sizeof(Vertex);
    UINT offset = 0;
    // set vertex buffer
    ID3D11Buffer* vbs[1] = { vertexBuffer };
    context->IASetVertexBuffers(0, 1, vbs, &stride, &offset);

    // set index buffer
    context->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R32_UINT, 0);

    // primitive topology (optional: ensure it is trianglelist)
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    UINT indexCount = static_cast<UINT>(indices.size());
    context->DrawIndexed(indexCount, 0, 0);
}

