// TexturePixelShader.hlsl
Texture2D tex0 : register(t0);
SamplerState samp0 : register(s0);

struct PS_INPUT
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD;
};

cbuffer TextureAlphaBuffer : register(b5)
{
    float Alpha;
    float3 Padding; // 16ƒoƒCƒg‘µ‚¦
}

float4 PSMain(PS_INPUT input) : SV_TARGET
{
    float4 col = tex0.Sample(samp0, input.texcoord);
    col.a *= Alpha;
    return col;
}
