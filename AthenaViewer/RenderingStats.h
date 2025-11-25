#pragma once

#include <atomic>
#include <memory>

namespace Athena {

    /**
     * レンダリング統計情報を追跡するクラス
     */
    class RenderingStats {
    public:
        RenderingStats();
        ~RenderingStats();

        /**
         * 描画コールを記録
         * @param vertexCount この描画コールで処理される頂点数
         * @param indexCount この描画コールで処理されるインデックス数
         */
        void RecordDrawCall(uint32_t vertexCount, uint32_t indexCount = 0);

        /**
         * インスタンス描画コールを記録
         * @param vertexCount 単一インスタンスの頂点数
         * @param instanceCount インスタンス数
         * @param indexCount この描画コールで処理されるインデックス数
         */
        void RecordDrawCallInstanced(uint32_t vertexCount, uint32_t instanceCount, uint32_t indexCount = 0);

        /**
         * フレーム開始時に統計をリセット
         */
        void BeginFrame();

        /**
         * フレーム終了時の処理
         */
        void EndFrame();

        /**
         * 現在のフレームの描画コール数を取得
         */
        uint32_t GetCurrentFrameDrawCalls() const;

        /**
         * 現在のフレームの頂点数を取得
         */
        uint32_t GetCurrentFrameVertexCount() const;

        /**
         * 現在のフレームのインデックス数を取得
         */
        uint32_t GetCurrentFrameIndexCount() const;

        /**
         * 前回のフレームの描画コール数を取得（UI表示用）
         */
        uint32_t GetLastFrameDrawCalls() const;

        /**
         * 前回のフレームの頂点数を取得（UI表示用）
         */
        uint32_t GetLastFrameVertexCount() const;

        /**
         * 前回のフレームのインデックス数を取得（UI表示用）
         */
        uint32_t GetLastFrameIndexCount() const;

    private:
        // 現在のフレーム統計
        std::atomic<uint32_t> currentFrameDrawCalls;
        std::atomic<uint32_t> currentFrameVertexCount;
        std::atomic<uint32_t> currentFrameIndexCount;

        // 前回のフレーム統計（UI表示用）
        uint32_t lastFrameDrawCalls;
        uint32_t lastFrameVertexCount;
        uint32_t lastFrameIndexCount;
    };

} // namespace Athena