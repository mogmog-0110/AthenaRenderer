#pragma once
#include <cstdio>
#include <cstdarg>
#include <windows.h>

namespace Athena {

    /**
     * @brief �V���v���ȃ��O�o�̓N���X
     *
     * �f�o�b�O����Visual Studio�̏o�̓E�B���h�E�ɕ\���B
     * OutputDebugString���g�p����Windows�W���̃f�o�b�O�o�͂ɑ��M�B
     */
    class Logger {
    public:
        static void Initialize() {
            // �����������i�����I�Ƀt�@�C���o�͂Ȃǂ�ǉ��\�j
        }

        static void Shutdown() {
            // �I������
        }

        /**
         * @brief ��񃍃O���o��
         * @param format printf�X�^�C���̃t�H�[�}�b�g������
         * @param ... �ϒ�����
         */
        static void Info(const char* format, ...) {
            char buffer[1024];
            va_list args;
            va_start(args, format);
            vsnprintf(buffer, sizeof(buffer), format, args);
            va_end(args);

            // Visual Studio�̏o�̓E�B���h�E�ɕ\��
            OutputDebugStringA("[INFO] ");
            OutputDebugStringA(buffer);
            OutputDebugStringA("\n");

            // �R���\�[���ɂ��o�́i�����I�ɃR���\�[���A�v�������ꍇ�p�j
            printf("[INFO] %s\n", buffer);
        }

        /**
         * @brief �x�����O���o��
         */
        static void Warning(const char* format, ...) {
            char buffer[1024];
            va_list args;
            va_start(args, format);
            vsnprintf(buffer, sizeof(buffer), format, args);
            va_end(args);

            OutputDebugStringA("[WARNING] ");
            OutputDebugStringA(buffer);
            OutputDebugStringA("\n");

            printf("[WARNING] %s\n", buffer);
        }

        /**
         * @brief �G���[���O���o��
         */
        static void Error(const char* format, ...) {
            char buffer[1024];
            va_list args;
            va_start(args, format);
            vsnprintf(buffer, sizeof(buffer), format, args);
            va_end(args);

            OutputDebugStringA("[ERROR] ");
            OutputDebugStringA(buffer);
            OutputDebugStringA("\n");

            printf("[ERROR] %s\n", buffer);
        }
    };

} // namespace Athena