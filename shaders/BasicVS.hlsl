/**
 * BasicVS.hlsl - 基本的な頂点シェーダー
 * 
 * 頂点シェーダー（Vertex Shader）は、各頂点に対して実行されるプログラムです。
 * 主な役割：
 * - 頂点座標の変換（ワールド座標→スクリーン座標）
 * - 頂点データの加工・補間準備
 */

// 頂点シェーダーへの入力
struct VSInput {
    float3 position : POSITION;  // 頂点座標（ローカル空間）
    float4 color    : COLOR;     // 頂点カラー
};

// 頂点シェーダーからの出力 = ピクセルシェーダーへの入力
struct VSOutput {
    float4 position : SV_POSITION;  // 変換後の座標（スクリーン空間）
    float4 color    : COLOR;        // カラー（ピクセルシェーダーに渡す）
};

/**
 * メイン関数
 * 
 * @param input 頂点データ
 * @return 変換後の頂点データ
 */
VSOutput main(VSInput input) {
    VSOutput output;
    
    // 座標変換（今回はそのまま）
    // 通常はここでモデル行列、ビュー行列、プロジェクション行列を掛ける
    output.position = float4(input.position, 1.0f);
    
    // カラーをそのまま渡す
    output.color = input.color;
    
    return output;
}
