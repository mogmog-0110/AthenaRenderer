// LightingPass Pixel Shader
// ディファードライティング用ピクセルシェーダー

// G-Bufferテクスチャ
Texture2D gbufferAlbedo : register(t0);  // アルベド + メタリック
Texture2D gbufferNormal : register(t1);  // 法線 + ラフネス
Texture2D gbufferDepth  : register(t2);  // 深度

SamplerState pointSampler : register(s0);

// ライト構造体
struct LightData
{
    float3 position;    // ライト位置（ポイントライト用）
    float  intensity;   // ライト強度
    float3 direction;   // ライト方向（方向ライト用）
    float  range;       // ライト範囲
    float3 color;       // ライト色
    int    type;        // ライトタイプ（0:方向, 1:ポイント, 2:スポット）
};

cbuffer LightingConstants : register(b0)
{
    float3    cameraPosition;   // カメラ位置
    float     exposure;         // 露出値
    float3    ambientColor;     // 環境光色
    uint      lightCount;       // ライト数
    LightData lights[8];        // 最大8個のライト
};

struct PSInput
{
    float4 position : SV_Position;
    float2 texcoord : TEXCOORD0;
};

// PBRライティング計算
float3 CalculatePBR(float3 albedo, float3 normal, float metallic, float roughness, 
                   float3 viewDir, float3 lightDir, float3 lightColor, float attenuation)
{
    // 基本的なPBRライティングの簡易実装
    float NdotL = max(0.0, dot(normal, lightDir));
    float NdotV = max(0.0, dot(normal, viewDir));
    
    // ディフューズ項（ランバート）
    float3 diffuse = albedo * (1.0 - metallic) / 3.14159;
    
    // スペキュラー項（簡易フレネル + D項）
    float3 halfVector = normalize(lightDir + viewDir);
    float NdotH = max(0.0, dot(normal, halfVector));
    float VdotH = max(0.0, dot(viewDir, halfVector));
    
    // 簡易フレネル（Schlick近似）
    float3 F0 = lerp(float3(0.04, 0.04, 0.04), albedo, metallic);
    float3 fresnel = F0 + (1.0 - F0) * pow(1.0 - VdotH, 5.0);
    
    // 分布項（GGX/Trowbridge-Reitz）
    float alpha = roughness * roughness;
    float alpha2 = alpha * alpha;
    float denom = NdotH * NdotH * (alpha2 - 1.0) + 1.0;
    float distribution = alpha2 / (3.14159 * denom * denom);
    
    // 幾何項（簡易版）
    float k = (roughness + 1.0) * (roughness + 1.0) / 8.0;
    float G1L = NdotL / (NdotL * (1.0 - k) + k);
    float G1V = NdotV / (NdotV * (1.0 - k) + k);
    float geometry = G1L * G1V;
    
    // BRDFの計算
    float3 specular = (distribution * geometry * fresnel) / max(0.0001, 4.0 * NdotL * NdotV);
    
    return (diffuse + specular) * lightColor * NdotL * attenuation;
}

// ワールド座標の復元
float3 ReconstructWorldPosition(float2 screenPos, float depth)
{
    // 簡易的な逆投影変換（実際にはビュー・プロジェクション逆行列が必要）
    // ここでは近似値を使用
    float4 clipPos = float4(screenPos * 2.0 - 1.0, depth, 1.0);
    clipPos.y = -clipPos.y; // Y座標反転
    
    // 簡易的なワールド座標復元（実際のプロジェクトでは正確な逆変換を実装）
    return clipPos.xyz * 10.0; // 仮の値
}

float4 main(PSInput input) : SV_Target
{
    // G-Bufferデータを読み取り
    float4 albedoData = gbufferAlbedo.Sample(pointSampler, input.texcoord);
    float4 normalData = gbufferNormal.Sample(pointSampler, input.texcoord);
    float depth = gbufferDepth.Sample(pointSampler, input.texcoord).r;
    
    // G-Bufferからデータを展開
    float3 albedo = albedoData.rgb;
    float metallic = albedoData.a;
    float3 worldNormal = normalize(normalData.rgb * 2.0 - 1.0);
    float roughness = normalData.a;
    
    // 深度値チェック（背景ピクセルはスキップ）
    if (depth >= 1.0) {
        return float4(0.0, 0.0, 0.0, 1.0);
    }
    
    // ワールド座標を復元
    float3 worldPos = ReconstructWorldPosition(input.texcoord, depth);
    float3 viewDir = normalize(cameraPosition - worldPos);
    
    // 環境光
    float3 finalColor = albedo * ambientColor;
    
    // 各ライトからの寄与を計算
    for (uint i = 0; i < lightCount; ++i)
    {
        LightData light = lights[i];
        float3 lightDir;
        float attenuation = 1.0;
        
        if (light.type == 0) // 方向ライト
        {
            lightDir = normalize(-light.direction);
            attenuation = 1.0;
        }
        else if (light.type == 1) // ポイントライト
        {
            float3 lightVec = light.position - worldPos;
            float distance = length(lightVec);
            lightDir = lightVec / distance;
            
            // 距離減衰
            attenuation = 1.0 / (1.0 + distance * distance / (light.range * light.range));
            attenuation = max(0.0, attenuation);
        }
        else
        {
            continue; // 未対応のライトタイプ
        }
        
        // PBRライティング計算
        float3 lightContribution = CalculatePBR(
            albedo, worldNormal, metallic, roughness,
            viewDir, lightDir, light.color, attenuation
        );
        
        finalColor += lightContribution * light.intensity;
    }
    
    // 露出調整
    finalColor *= exposure;
    
    return float4(finalColor, 1.0);
}