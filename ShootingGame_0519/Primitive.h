#pragma once
#include <d3d11.h>
#include <DirectXMath.h>
#include <vector>

using namespace DirectX;

//頂点情報をまとめた構造体
struct Vertex
{
    XMFLOAT3 position;  //頂点座標
    XMFLOAT3 normal;    //法線
    XMFLOAT4 color;     //色
    XMFLOAT2 texcoord;  //テクスチャ座標
};

//簡単な図形(球体や箱など)を管理するクラス
class Primitive
{
public:
    Primitive() {};
    virtual ~Primitive()
    {
        if (vertexBuffer) vertexBuffer->Release();
        if (indexBuffer) indexBuffer->Release();
    };
    
    //球体を生成する関数
    //radius: 半径, sliceCount:横方向の分割数, stackCount:縦方向の分割数
    void CreateSphere(ID3D11Device* device, float radius, int sliceCount, int stackCount);

    //箱を生成する関数
    void CreateBox   (ID3D11Device* device, float width, float height, float depth);

    //板を生成する関数
    void CreatePlane(ID3D11Device* device,float width, float depth,int subdivisionsX = 1, int subdivisionsZ = 1,const DirectX::XMFLOAT4& color = { 1,1,1,1 },bool generateUVs = true);

    //描画
    void Draw(ID3D11DeviceContext* context);

    

private:
    std::vector<Vertex> vertices;   //頂点情報を保持する動的配列
    std::vector<uint32_t> indices;  //インデックス情報（頂点の接続順）

    ID3D11Buffer* vertexBuffer = nullptr; //頂点バッファ（GPU用）
    ID3D11Buffer* indexBuffer  = nullptr;  //インデックスバッファ（GPU用）

    //頂点バッファとインデックスバッファを作成する関数
    void CreateBuffers(ID3D11Device* device);
};

