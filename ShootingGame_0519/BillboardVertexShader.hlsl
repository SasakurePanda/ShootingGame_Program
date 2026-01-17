struct VS_INPUT
{
    float3 position : POSITION;
    float2 texcoord : TEXCOORD;
    float4 color : COLOR;
};

struct VS_OUTPUT
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
    float4 color : COLOR0;
};

cbuffer WorldBuffer : register(b0)
{
    matrix world;
};

cbuffer ViewBuffer : register(b1)
{
    matrix view;
};

cbuffer ProjectionBuffer : register(b2)
{
    matrix projection;
};

VS_OUTPUT VSMain(VS_INPUT input)
{
    VS_OUTPUT o;

    float4 pos = float4(input.position, 1.0f);
    pos = mul(pos, world);
    pos = mul(pos, view);
    pos = mul(pos, projection);

    o.position = pos;
    o.texcoord = input.texcoord;
    o.color = input.color;
    return o;
}