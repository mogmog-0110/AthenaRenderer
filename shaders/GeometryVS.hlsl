// GeometryPass Vertex Shader
// 基本的な3Dオブジェクトのレンダリング用頂点シェーダー

cbuffer GeometryConstants : register(b0)
{
    matrix mvpMatrix;        // Model-View-Projection行列
    matrix worldMatrix;      // ワールド行列
    matrix viewMatrix;       // ビュー行列
    matrix projMatrix;       // プロジェクション行列
    float3 cameraPosition;   // カメラ位置
    float  padding1;
    float3 lightDirection;   // ライト方向
    float  padding2;
    float3 lightColor;       // ライト色
    float  padding3;
};

struct VSInput
{
    float3 position : POSITION;
    float3 normal   : NORMAL;
    float2 texcoord : TEXCOORD0;
};

struct VSOutput
{
    float4 position     : SV_Position;
    float3 worldPos     : POSITION;
    float3 normal       : NORMAL;
    float2 texcoord     : TEXCOORD0;
    float3 viewDir      : TEXCOORD1;
    float3 lightDir     : TEXCOORD2;
};

VSOutput main(VSInput input)
{
    VSOutput output;
    
    float4 worldPos = mul(float4(input.position, 1.0), worldMatrix);
    output.worldPos = worldPos.xyz;
    
    output.position = mul(float4(input.position, 1.0), mvpMatrix);
    
    output.normal = normalize(mul(input.normal, (float3x3)worldMatrix));
    
    output.texcoord = input.texcoord;
    
    output.viewDir = normalize(cameraPosition - worldPos.xyz);
    
    output.lightDir = normalize(-lightDirection);
    
    return output;
}