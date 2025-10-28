#pragma once

namespace Athena {

    /**
     * @brief ログ出力システム
     *
     * Visual Studioの出力ウィンドウとファイルの両方にログを出力。
     * シングルトンパターンで実装されており、プログラム全体で共有。
     */
    class Logger {
    public:
        /**
         * @brief ログシステムを初期化
         *
         * ログファイル（athena_log.txt）を作成し、ログ出力を開始。
         * プログラムの最初に一度だけ呼び出す。
         */
        static void Initialize();

        /**
         * @brief ログシステムをシャットダウン
         *
         * ログファイルを閉じ、リソースを解放。
         * プログラムの最後に一度だけ呼び出す。
         */
        static void Shutdown();

        /**
         * @brief 情報ログを出力
         *
         * @param format printfスタイルのフォーマット文字列
         * @param ... 可変長引数
         *
         * 例: Logger::Info("Device initialized: %s", deviceName);
         */
        static void Info(const char* format, ...);

        /**
         * @brief 警告ログを出力
         *
         * @param format printfスタイルのフォーマット文字列
         * @param ... 可変長引数
         */
        static void Warning(const char* format, ...);

        /**
         * @brief エラーログを出力
         *
         * @param format printfスタイルのフォーマット文字列
         * @param ... 可変長引数
         */
        static void Error(const char* format, ...);

    private:
        static void GetTimeString(char* buffer, size_t bufferSize);
    };

} // namespace Athena