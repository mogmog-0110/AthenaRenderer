// TexturedVS.hlsl - テクスチャ付きキューブ用の頂点シェーダー（修正版）

// 定数バッファ（b0レジスタ）
cbuffer TransformBuffer : register(b0)
{
    float4x4 mvp;  // Model-View-Projection 行列
};

// 入力構造体
struct VSInput
{
    float3 position : POSITION;  // 頂点位置
    float2 texcoord : TEXCOORD;  // テクスチャ座標
};

// 出力構造体（ピクセルシェーダーへの入力）
struct PSInput
{
    float4 position : SV_POSITION;  // クリップ空間の位置
    float2 texcoord : TEXCOORD;     // テクスチャ座標
};

// メイン関数
PSInput main(VSInput input)
{
    PSInput output;
    
    output.position = mul(float4(input.position, 1.0f), mvp);
    
    // テクスチャ座標はそのまま渡す
    output.texcoord = input.texcoord;
    
    return output;
}
