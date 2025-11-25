#include "RenderingStats.h"

namespace Athena {

    RenderingStats::RenderingStats() 
        : currentFrameDrawCalls(0), currentFrameVertexCount(0), currentFrameIndexCount(0),
          lastFrameDrawCalls(0), lastFrameVertexCount(0), lastFrameIndexCount(0) {
    }

    RenderingStats::~RenderingStats() {
    }

    void RenderingStats::RecordDrawCall(uint32_t vertexCount, uint32_t indexCount) {
        currentFrameDrawCalls++;
        currentFrameVertexCount += vertexCount;
        currentFrameIndexCount += indexCount;
    }

    void RenderingStats::RecordDrawCallInstanced(uint32_t vertexCount, uint32_t instanceCount, uint32_t indexCount) {
        currentFrameDrawCalls++;
        currentFrameVertexCount += (vertexCount * instanceCount);
        currentFrameIndexCount += (indexCount * instanceCount);
    }

    void RenderingStats::BeginFrame() {
        // 前回のフレーム統計を保存
        lastFrameDrawCalls = currentFrameDrawCalls.load();
        lastFrameVertexCount = currentFrameVertexCount.load();
        lastFrameIndexCount = currentFrameIndexCount.load();

        // 現在のフレーム統計をリセット
        currentFrameDrawCalls = 0;
        currentFrameVertexCount = 0;
        currentFrameIndexCount = 0;
    }

    void RenderingStats::EndFrame() {
        // フレーム終了時の処理（現在は空）
    }

    uint32_t RenderingStats::GetCurrentFrameDrawCalls() const {
        return currentFrameDrawCalls.load();
    }

    uint32_t RenderingStats::GetCurrentFrameVertexCount() const {
        return currentFrameVertexCount.load();
    }

    uint32_t RenderingStats::GetCurrentFrameIndexCount() const {
        return currentFrameIndexCount.load();
    }

    uint32_t RenderingStats::GetLastFrameDrawCalls() const {
        return lastFrameDrawCalls;
    }

    uint32_t RenderingStats::GetLastFrameVertexCount() const {
        return lastFrameVertexCount;
    }

    uint32_t RenderingStats::GetLastFrameIndexCount() const {
        return lastFrameIndexCount;
    }

} // namespace Athena