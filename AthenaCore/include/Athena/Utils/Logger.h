#pragma once

namespace Athena {

    /**
     * @brief ���O�o�̓V�X�e��
     *
     * Visual Studio�̏o�̓E�B���h�E�ƃt�@�C���̗����Ƀ��O���o�́B
     * �V���O���g���p�^�[���Ŏ�������Ă���A�v���O�����S�̂ŋ��L�B
     */
    class Logger {
    public:
        /**
         * @brief ���O�V�X�e����������
         *
         * ���O�t�@�C���iathena_log.txt�j���쐬���A���O�o�͂��J�n�B
         * �v���O�����̍ŏ��Ɉ�x�����Ăяo���B
         */
        static void Initialize();

        /**
         * @brief ���O�V�X�e�����V���b�g�_�E��
         *
         * ���O�t�@�C������A���\�[�X������B
         * �v���O�����̍Ō�Ɉ�x�����Ăяo���B
         */
        static void Shutdown();

        /**
         * @brief ��񃍃O���o��
         *
         * @param format printf�X�^�C���̃t�H�[�}�b�g������
         * @param ... �ϒ�����
         *
         * ��: Logger::Info("Device initialized: %s", deviceName);
         */
        static void Info(const char* format, ...);

        /**
         * @brief �x�����O���o��
         *
         * @param format printf�X�^�C���̃t�H�[�}�b�g������
         * @param ... �ϒ�����
         */
        static void Warning(const char* format, ...);

        /**
         * @brief �G���[���O���o��
         *
         * @param format printf�X�^�C���̃t�H�[�}�b�g������
         * @param ... �ϒ�����
         */
        static void Error(const char* format, ...);

    private:
        static void GetTimeString(char* buffer, size_t bufferSize);
    };

} // namespace Athena