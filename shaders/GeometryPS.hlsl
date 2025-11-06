// GeometryPass Pixel Shader
// 基本的な3Dオブジェクトのレンダリング用ピクセルシェーダー

Texture2D    mainTexture : register(t0);
SamplerState mainSampler : register(s0);

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
    uint   textureIndex;
    uint   materialIndex;
    uint   objectID;
    float  paddingArray[26];
};

struct PSInput
{
    float4 position     : SV_Position;
    float3 worldPos     : POSITION;
    float3 normal       : NORMAL;
    float2 texcoord     : TEXCOORD0;
    float3 viewDir      : TEXCOORD1;
    float3 lightDir     : TEXCOORD2;
};

float4 main(PSInput input) : SV_Target
{
    float4 albedo = mainTexture.Sample(mainSampler, input.texcoord);
    
    float3 normal = normalize(input.normal);
    float3 lightDir = normalize(input.lightDir);
    float3 viewDir = normalize(input.viewDir);
    
    float NdotL = max(0.0, dot(normal, lightDir));
    float3 diffuse = lightColor * NdotL;
    
    float3 halfVector = normalize(lightDir + viewDir);
    float NdotH = max(0.0, dot(normal, halfVector));
    float specularPower = 32.0;
    float specular = pow(NdotH, specularPower);
    
    float3 ambient = float3(0.1, 0.1, 0.1);
    
    float3 idColor = float3(
        ((objectID & 1) > 0) ? 0.2 : 0.0,
        ((objectID & 2) > 0) ? 0.2 : 0.0,
        ((objectID & 4) > 0) ? 0.2 : 0.0
    );
    
    float3 finalColor = albedo.rgb * (ambient + diffuse) + lightColor * specular * 0.3 + idColor;
    
    return float4(finalColor, albedo.a);
}