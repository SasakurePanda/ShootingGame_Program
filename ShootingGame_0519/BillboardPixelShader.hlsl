Texture2D tex0 : register(t0);
SamplerState samp0 : register(s0);

struct PS_INPUT
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
    float4 color : COLOR0;
};

float4 PSMain(PS_INPUT input) : SV_TARGET
{
    float4 col = tex0.Sample(samp0, input.texcoord);
    col *= input.color; // 色とアルファを掛ける
    return col;
}
