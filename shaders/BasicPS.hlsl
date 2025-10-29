/**
 * BasicPS.hlsl - 基本的なピクセルシェーダー
 * 
 * ピクセルシェーダー（Pixel Shader / Fragment Shader）は、
 * 各ピクセルに対して実行されるプログラムです。
 * 主な役割：
 * - ピクセルの最終的な色を決定
 * - ライティング計算、テクスチャサンプリングなど
 */

// ピクセルシェーダーへの入力（頂点シェーダーの出力と同じ構造）
struct PSInput {
    float4 position : SV_POSITION;  // スクリーン座標（自動補間済み）
    float4 color    : COLOR;        // カラー（頂点間で補間済み）
};

/**
 * メイン関数
 * 
 * @param input 補間されたピクセルデータ
 * @return 最終的なピクセルの色
 * 
 * SV_Target: このピクセルシェーダーの出力が
 *            レンダーターゲット（描画先）に書き込まれることを示す
 */
float4 main(PSInput input) : SV_Target {
    // 補間されたカラーをそのまま返す
    return input.color;
}
