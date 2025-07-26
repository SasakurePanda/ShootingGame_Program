cbuffer WorldMatrix : register(b0)
{
    matrix g_World;
};

cbuffer ViewMatrix : register(b1)
{
    matrix g_View;
};

cbuffer ProjectionMatrix : register(b2)
{
    matrix g_Projection;
};

struct VS_Input
{
    float3 pos : POSITION; // 頂点座標
    float2 uv : TEXCOORD; // UV座標（グリッド模様に使う）
};

struct VS_Output
{
    float4 pos : SV_POSITION; // クリップ空間位置
    float2 uv : TEXCOORD0; // ピクセルシェーダーに渡すUV
};

VS_Output VSMain(VS_Input input)
{
    VS_Output output;

    // 行列変換：ワールド → ビュー → プロジェクション
    float4 worldPos = mul(float4(input.pos, 1.0f), g_World);
    float4 viewPos = mul(worldPos, g_View);
    output.pos = mul(viewPos, g_Projection);

    // UVをそのまま渡す（ピクセルシェーダー側で使用）
    output.uv = input.uv;

    return output;
}
