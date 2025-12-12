// MotionBlurPS.hlsl

Texture2D gSceneTex : register(t0); //今フレームのシーンテクスチャ
Texture2D gPrevSceneTex : register(t1); //前フレームのシーンテクスチャ
SamplerState gSampler : register(s0); // サンプラーステート

//ポストエフェクト共通定数バッファ
cbuffer PostProcessCB : register(b0) 
{
    float motionBlurAmount;     //0～1
    float2 motionBlurDir;       //画面上の方向ベクトル
    float motionBlurStretch;    //

    float bloomAmount;    //
    float vignetteAmount; //
};

//フルスクリーンクアッドから渡される頂点データ
struct VS_OUT
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD0;
};

//サンプル数
static const int SAMPLE_COUNT = 8;

//ピクセルシェーダーメイン関数
float4 PSMain(VS_OUT input) : SV_TARGET
{
    float2 uv = input.uv;

    //ブラー強度がほぼ 0 ならそのまま返す
    if (motionBlurAmount <= 0.001f)
    {
        return gSceneTex.Sample(gSampler, uv);
    }

    //外枠マスク（中心はブラー0、外側だけブラー1）
    float2 d = abs(uv - float2(0.5, 0.5));
    
    // inner: ここまではブラー0
    float inner =  0.1f; 
    // outer: ここからはフルブラー
    float outer = 0.65f;
    
    float edgeFactorX = saturate((d.x - inner) / (outer - inner));
    float edgeFactorY = saturate((d.y - inner) / (outer - inner));
    
    //XかYどちらかが外側に寄っていればブラー強め（四角い額縁っぽいマスク）
    float edgeFactor = max(edgeFactorX, edgeFactorY);

    //実際に使うブラー強度 = 全体のブラー量 × 外枠マスク
    float effectiveBlur = motionBlurAmount * edgeFactor;

    if (effectiveBlur <= 0.001f)
    {
        // 中央付近はほぼブラーなし
        return gSceneTex.Sample(gSampler, uv);
    }

    //--------------------------------------
    // 2) 伸びる方向（motionBlurDir を使う）
    //--------------------------------------
    float2 dir = motionBlurDir;
    if (abs(dir.x) + abs(dir.y) < 1e-3f)
    {
        // もし 0,0 が来た時の保険
        dir = float2(0.0f, -1.0f); // 画面奥方向に伸ばすイメージ
    }
    dir = normalize(dir);

    float2 offsetBase = dir * motionBlurStretch * effectiveBlur;

    //--------------------------------------
    // 3) サンプリング（簡単な等重みブラー）
    //--------------------------------------
    float3 col = 0;
    float wSum = 0;

    [unroll]
    for (int i = 0; i < SAMPLE_COUNT; ++i)
    {
        float t = (i / (SAMPLE_COUNT - 1.0f)) - 0.5f; // -0.5 ～ +0.5
        float2 offset = offsetBase * t;
        float w = 1.0f;

        col += gSceneTex.Sample(gSampler, uv + offset).rgb * w;
        wSum += w;
    }

    col /= max(wSum, 1e-5f);

    return float4(col, 1.0f);
}
