// CubePS.hlsl - キューブ用ピクセルシェーダー

// 入力構造体
struct PSInput
{
    float4 position : SV_POSITION;
    float3 color : COLOR;
};

// メイン関数
float4 main(PSInput input) : SV_TARGET
{
    // 頂点シェーダーから受け取った色をそのまま出力
    return float4(input.color, 1.0f);
}
