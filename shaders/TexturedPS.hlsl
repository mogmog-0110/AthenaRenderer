// TexturedPS.hlsl - テクスチャ付きキューブ用のピクセルシェーダー

// テクスチャとサンプラー
Texture2D    g_texture : register(t0);  // テクスチャ（SRV）
SamplerState g_sampler : register(s0);  // サンプラー

// 入力構造体（頂点シェーダーからの出力）
struct PSInput
{
    float4 position : SV_POSITION;  // クリップ空間の位置（使用しない）
    float2 texcoord : TEXCOORD;     // テクスチャ座標
};

// メイン関数
float4 main(PSInput input) : SV_TARGET
{
    // テクスチャからカラーをサンプリング
    float4 color = g_texture.Sample(g_sampler, input.texcoord);
    
    return color;
}
