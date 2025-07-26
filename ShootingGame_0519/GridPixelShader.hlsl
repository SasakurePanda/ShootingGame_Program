struct PS_Input
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD0;
};

float4 PSMain(PS_Input input):SV_TARGET
{
    //グリッドの線の太さ
    float gridSize = 10.0f; // 10等分（1.0 / 10.0 間隔）
    float lineWidth = 0.02f; // 線の太さ（0.0〜1.0のUV空間で）

    // UVを使って、格子の線に近いかどうかを判定
    float2 gridUV = frac(input.uv * gridSize); // 0.0〜1.0でループさせる

    // 線に近いところだけ黒、それ以外は薄いグレー
    float2 gridLine = step(gridUV, lineWidth) + step(1.0 - gridUV, lineWidth);

    float lineMask = saturate(gridLine.x + gridLine.y); // 0 or 1

    float4 lineColor = float4(0.1, 0.1, 0.1, 1.0); // 黒っぽいグリッド線
    float4 baseColor = float4(0.7, 0.7, 0.7, 1.0); // 明るめの床ベース

    return lerp(baseColor, lineColor, lineMask); // 線があるところだけ黒く
}
