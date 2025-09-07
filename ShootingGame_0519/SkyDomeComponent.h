#pragma once
#include "Component.h"
#include <d3d11.h>
#include <DirectXMath.h>
#include <vector>

class SkyDomeComponent : public Component
{
public:
    //コンストラクト
    SkyDomeComponent(ID3D11Device* device,
        float radius = 1.0f,
        UINT  latSegments = 32,
        UINT  longSegments = 64);

    //デストラクト
    virtual ~SkyDomeComponent() = default;

    void Initialize() override {};
    void Update(float dt) override {};
    void Draw(float alpha) override {};

private:
    //頂点用の構造体
    struct Vertex
    {
        DirectX::XMFLOAT3 position;
        DirectX::XMFLOAT3 normal;
        DirectX::XMFLOAT2 uv;
    };

    //SkyDome用のパラメータ
    float m_radius;         //半径
    UINT  m_latSegs;        //緯度分割数
    UINT longSegments = 64; //経度分割数

    //--------------D3D関連のリソース変数群--------------
    ID3D11Device* m_device = nullptr;
    ID3D11Buffer* m_vb = nullptr;
    ID3D11Buffer* m_ib = nullptr;
    ID3D11Buffer* m_cbViewProj = nullptr;
    ID3D11VertexShader* m_vs = nullptr;
    ID3D11PixelShader* m_ps = nullptr;
    ID3D11InputLayout* m_inputLayout = nullptr;
    ID3D11ShaderResourceView* m_srv = nullptr;
    ID3D11SamplerState* m_sampler = nullptr;
    ID3D11DepthStencilState* m_dsState = nullptr;
    ID3D11RasterizerState* m_rsState = nullptr;
    //---------------------------------------------------

    //メッシュ生成データ関連
    std::vector<Vertex>      m_vertices;
    std::vector<UINT>        m_indices;

    //インデックス等の生成関数関連
    void GenerateMesh();    //緯度経度ループで頂点・インデックス生成関数
    void CreateBuffers();   //VB/IB/定数バッファ作成関数
    void LoadShaders();     //HLSL の VS/PS, InputLayout 作成関数
    void CreateStates();    //SamplerState, DepthStencilState, RasterizerState 作成関数
};