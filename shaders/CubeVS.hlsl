// CubeVS.hlsl - キューブ用頂点シェーダー

// 定数バッファ（MVP行列）
cbuffer ConstantBuffer : register(b0)
{
    float4x4 mvp;
};

// 入力構造体
struct VSInput
{
    float3 position : POSITION;
    float3 color : COLOR;
};

// 出力構造体
struct VSOutput
{
    float4 position : SV_POSITION;
    float3 color : COLOR;
};

// メイン関数
VSOutput main(VSInput input)
{
    VSOutput output;
    
    // MVP行列による座標変換
    output.position = mul(float4(input.position, 1.0f), mvp);
    
    // 色をそのまま渡す
    output.color = input.color;
    
    return output;
}
