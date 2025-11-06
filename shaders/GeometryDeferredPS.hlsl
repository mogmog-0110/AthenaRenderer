// G-Buffer Generation Pixel Shader for Deferred Rendering
// Multiple Render Targets (MRT) output for deferred shading pipeline

Texture2D    mainTexture : register(t0);
SamplerState mainSampler : register(s0);

cbuffer GeometryConstants : register(b0)
{
    matrix mvpMatrix;
    matrix worldMatrix;
    matrix viewMatrix;
    matrix projMatrix;
    float3 cameraPosition;
    float  padding1;
    float3 lightDirection;
    float  padding2;
    float3 lightColor;
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

struct PSOutput
{
    float4 albedo    : SV_Target0;  // RGB: Albedo, A: Metallic
    float4 normal    : SV_Target1;  // RGB: World Space Normal, A: Roughness
    float4 material  : SV_Target2;  // R: AO, G: Unused, B: Unused, A: Object ID
};

PSOutput main(PSInput input) : SV_Target
{
    PSOutput output;
    
    // Sample albedo texture
    float4 albedoSample = mainTexture.Sample(mainSampler, input.texcoord);
    
    // Normalize world space normal
    float3 worldNormal = normalize(input.normal);
    
    // Pack G-Buffer data
    
    // Target 0: Albedo + Metallic
    output.albedo.rgb = albedoSample.rgb;
    output.albedo.a = 0.0f; // Default metallic value (non-metallic)
    
    // Target 1: World Normal + Roughness
    // Pack normal from [-1,1] to [0,1] range
    output.normal.rgb = worldNormal * 0.5f + 0.5f;
    output.normal.a = 0.5f; // Default roughness value
    
    // Target 2: Material parameters
    output.material.r = 1.0f; // AO (full ambient occlusion)
    output.material.g = 0.0f; // Unused
    output.material.b = 0.0f; // Unused
    output.material.a = float(objectID) / 255.0f; // Object ID normalized
    
    // デバッグ: G-Bufferが正常に書き込まれているかテスト用の明るい色を出力
    // output.albedo = float4(1.0f, 0.0f, 1.0f, 1.0f);  // マゼンタ色でデバッグ
    
    return output;
}