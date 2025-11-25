#pragma once

#include <Windows.h>
#include <psapi.h>

namespace Athena {

    class SimpleStats {
    public:
        SimpleStats() : drawCalls(0), vertexCount(0), memoryUsageMB(0.0f) {}

        void RecordDrawCall(int vertices) {
            drawCalls++;
            vertexCount += vertices;
        }

        void BeginFrame() {
            drawCalls = 0;
            vertexCount = 0;
            UpdateMemoryUsage();
        }

        int GetDrawCalls() const { return drawCalls; }
        int GetVertexCount() const { return vertexCount; }
        float GetMemoryUsageMB() const { return memoryUsageMB; }

    private:
        int drawCalls;
        int vertexCount;
        float memoryUsageMB;

        void UpdateMemoryUsage() {
            PROCESS_MEMORY_COUNTERS pmc;
            if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
                memoryUsageMB = static_cast<float>(pmc.WorkingSetSize) / (1024.0f * 1024.0f);
            }
        }
    };

} // namespace Athena