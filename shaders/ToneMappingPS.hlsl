// ToneMappingPass Pixel Shader
// HDRからLDRへのトーンマッピング

Texture2D hdrTexture : register(t0);
SamplerState linearSampler : register(s0);

cbuffer ToneMappingConstants : register(b0)
{
    float exposure;             // 露出値
    float gamma;                // ガンマ値
    int   toneMappingMethod;    // トーンマッピング手法
    float whitePoint;           // ホワイトポイント
    float contrast;             // コントラスト
    float brightness;           // 明度
    float saturation;           // 彩度
    float padding;
};

struct PSInput
{
    float4 position : SV_Position;
    float2 texcoord : TEXCOORD0;
};

// ACES Filmic Tone Mapping
float3 ACESFilm(float3 x)
{
    float a = 2.51;
    float b = 0.03;
    float c = 2.43;
    float d = 0.59;
    float e = 0.14;
    return saturate((x * (a * x + b)) / (x * (c * x + d) + e));
}

// Reinhard Tone Mapping
float3 Reinhard(float3 color)
{
    return color / (color + 1.0);
}

// Reinhard Luminance Tone Mapping
float3 ReinhardLuma(float3 color)
{
    float luma = dot(color, float3(0.299, 0.587, 0.114));
    float toneMappedLuma = luma / (luma + 1.0);
    return color * (toneMappedLuma / luma);
}

// Uncharted 2 Filmic Tone Mapping
float3 Uncharted2Tonemap(float3 x)
{
    float A = 0.15;
    float B = 0.50;
    float C = 0.10;
    float D = 0.20;
    float E = 0.02;
    float F = 0.30;
    
    return ((x * (A * x + C * B) + D * E) / (x * (A * x + B) + D * F)) - E / F;
}

// RGB to HSV conversion
float3 RGBtoHSV(float3 c)
{
    float4 K = float4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0);
    float4 p = lerp(float4(c.bg, K.wz), float4(c.gb, K.xy), step(c.b, c.g));
    float4 q = lerp(float4(p.xyw, c.r), float4(c.r, p.yzx), step(p.x, c.r));
    
    float d = q.x - min(q.w, q.y);
    float e = 1.0e-10;
    return float3(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x);
}

// HSV to RGB conversion
float3 HSVtoRGB(float3 c)
{
    float4 K = float4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    float3 p = abs(frac(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * lerp(K.xxx, saturate(p - K.xxx), c.y);
}

// Color grading function
float3 ApplyColorGrading(float3 color)
{
    // コントラスト調整
    color = pow(color, 1.0 / contrast);
    
    // 明度調整
    color += brightness;
    
    // 彩度調整
    if (saturation != 1.0)
    {
        float3 hsv = RGBtoHSV(color);
        hsv.y *= saturation;
        color = HSVtoRGB(hsv);
    }
    
    return color;
}

float4 main(PSInput input) : SV_Target
{
    // HDRテクスチャをサンプリング
    float3 hdrColor = hdrTexture.Sample(linearSampler, input.texcoord).rgb;
    
    // 露出調整
    hdrColor *= exposure;
    
    // トーンマッピング適用
    float3 toneMappedColor;
    
    switch (toneMappingMethod)
    {
        case 0: // Linear (clamp)
            toneMappedColor = saturate(hdrColor);
            break;
            
        case 1: // Reinhard
            toneMappedColor = Reinhard(hdrColor);
            break;
            
        case 2: // Reinhard Luminance
            toneMappedColor = ReinhardLuma(hdrColor);
            break;
            
        case 3: // ACES
            toneMappedColor = ACESFilm(hdrColor);
            break;
            
        case 4: // Uncharted 2
            {
                float3 curr = Uncharted2Tonemap(hdrColor);
                float3 whiteScale = 1.0 / Uncharted2Tonemap(whitePoint);
                toneMappedColor = curr * whiteScale;
            }
            break;
            
        default:
            toneMappedColor = ACESFilm(hdrColor);
            break;
    }
    
    // 色調補正適用
    toneMappedColor = ApplyColorGrading(toneMappedColor);
    
    // ガンマ補正
    toneMappedColor = pow(toneMappedColor, 1.0 / gamma);
    
    // 最終的なクランプ
    toneMappedColor = saturate(toneMappedColor);
    
    return float4(toneMappedColor, 1.0);
}