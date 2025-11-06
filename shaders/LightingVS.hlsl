// LightingPass Vertex Shader
// フルスクリーンクアッド用頂点シェーダー
// 頂点バッファを使わずに大きな三角形を生成

struct VSOutput
{
    float4 position : SV_Position;
    float2 texcoord : TEXCOORD0;
};

VSOutput main(uint vertexID : SV_VertexID)
{
    VSOutput output;
    
    // 頂点IDに基づいてフルスクリーンクアッドを生成
    // 大きな三角形を描画してスクリーン全体をカバー
    output.texcoord = float2((vertexID << 1) & 2, vertexID & 2);
    output.position = float4(output.texcoord * 2.0 - 1.0, 0.0, 1.0);
    
    // Y座標を反転（DirectXのテクスチャ座標系に合わせる）
    output.position.y = -output.position.y;
    
    return output;
}