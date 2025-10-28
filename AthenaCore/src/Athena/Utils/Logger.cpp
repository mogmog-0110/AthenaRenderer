#include "Athena/Utils/Logger.h"
#include <Windows.h>
#include <cstdio>
#include <cstdarg>
#include <ctime>

namespace Athena {

    namespace {
        bool s_Initialized = false;
        FILE* s_LogFile = nullptr;
    }

    void Logger::Initialize() {
        if (s_Initialized) {
            return;
        }

        // ���O�t�@�C�����J���i�ǋL���[�h�j
        fopen_s(&s_LogFile, "athena_log.txt", "w");

        s_Initialized = true;

        Info("===========================================");
        Info("Athena Renderer - Log Started");
        Info("===========================================");
    }

    void Logger::Shutdown() {
        if (!s_Initialized) {
            return;
        }

        Info("===========================================");
        Info("Athena Renderer - Log Ended");
        Info("===========================================");

        if (s_LogFile) {
            fclose(s_LogFile);
            s_LogFile = nullptr;
        }

        s_Initialized = false;
    }

    void Logger::Info(const char* format, ...) {
        if (!s_Initialized) {
            return;
        }

        // �ϒ������̏���
        va_list args;
        va_start(args, format);

        // �^�C���X�^���v�擾
        char timeStr[32];
        GetTimeString(timeStr, sizeof(timeStr));

        // �t�H�[�}�b�g������̍쐬
        char buffer[1024];
        vsnprintf(buffer, sizeof(buffer), format, args);
        va_end(args);

        // �R���\�[���o�́iVisual Studio�̏o�̓E�B���h�E�j
        char output[1200];
        snprintf(output, sizeof(output), "[%s][INFO] %s\n", timeStr, buffer);
        OutputDebugStringA(output);

        // �t�@�C���o��
        if (s_LogFile) {
            fprintf(s_LogFile, "%s", output);
            fflush(s_LogFile);
        }
    }

    void Logger::Warning(const char* format, ...) {
        if (!s_Initialized) {
            return;
        }

        va_list args;
        va_start(args, format);

        char timeStr[32];
        GetTimeString(timeStr, sizeof(timeStr));

        char buffer[1024];
        vsnprintf(buffer, sizeof(buffer), format, args);
        va_end(args);

        char output[1200];
        snprintf(output, sizeof(output), "[%s][WARN] %s\n", timeStr, buffer);
        OutputDebugStringA(output);

        if (s_LogFile) {
            fprintf(s_LogFile, "%s", output);
            fflush(s_LogFile);
        }
    }

    void Logger::Error(const char* format, ...) {
        if (!s_Initialized) {
            return;
        }

        va_list args;
        va_start(args, format);

        char timeStr[32];
        GetTimeString(timeStr, sizeof(timeStr));

        char buffer[1024];
        vsnprintf(buffer, sizeof(buffer), format, args);
        va_end(args);

        char output[1200];
        snprintf(output, sizeof(output), "[%s][ERROR] %s\n", timeStr, buffer);
        OutputDebugStringA(output);

        if (s_LogFile) {
            fprintf(s_LogFile, "%s", output);
            fflush(s_LogFile);
        }
    }

    void Logger::GetTimeString(char* buffer, size_t bufferSize) {
        time_t now = time(nullptr);
        tm timeInfo;
        localtime_s(&timeInfo, &now);
        strftime(buffer, bufferSize, "%H:%M:%S", &timeInfo);
    }

} // namespace Athena