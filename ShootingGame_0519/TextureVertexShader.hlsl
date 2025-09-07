// TextureVertexShader.hlsl
cbuffer WorldBuffer : register(b0)
{
    matrix world; // 使用はしないがシグネチャ合わせのため
};
cbuffer ViewBuffer : register(b1)
{
    matrix view;
};
cbuffer ProjectionBuffer : register(b2)
{
    matrix projection;
};

struct VS_INPUT
{
    float3 position : POSITION;
    float2 texcoord : TEXCOORD;
};

struct VS_OUTPUT
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD;
};

VS_OUTPUT VSMain(VS_INPUT input)
{
    VS_OUTPUT o;
    // 2D の場合、SetWorldViewProjection2D() で既に
    // world/view/projection を正射影にしている前提です。
    float4 pos = float4(input.position, 1.0f);
    // 乗算順はシステムに合わせてください（ここでは projection * view * world * pos）
    o.position = mul(pos, world);
    o.position = mul(o.position, view);
    o.position = mul(o.position, projection);
    o.texcoord = input.texcoord;
    return o;
}
