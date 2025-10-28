#pragma once
#include <cstdio>
#include <cstdarg>
#include <windows.h>

namespace Athena {

    /**
     * @brief シンプルなログ出力クラス
     *
     * デバッグ情報をVisual Studioの出力ウィンドウに表示。
     * OutputDebugStringを使用してWindows標準のデバッグ出力に送信。
     */
    class Logger {
    public:
        static void Initialize() {
            // 初期化処理（将来的にファイル出力などを追加可能）
        }

        static void Shutdown() {
            // 終了処理
        }

        /**
         * @brief 情報ログを出力
         * @param format printfスタイルのフォーマット文字列
         * @param ... 可変長引数
         */
        static void Info(const char* format, ...) {
            char buffer[1024];
            va_list args;
            va_start(args, format);
            vsnprintf(buffer, sizeof(buffer), format, args);
            va_end(args);

            // Visual Studioの出力ウィンドウに表示
            OutputDebugStringA("[INFO] ");
            OutputDebugStringA(buffer);
            OutputDebugStringA("\n");

            // コンソールにも出力（将来的にコンソールアプリを作る場合用）
            printf("[INFO] %s\n", buffer);
        }

        /**
         * @brief 警告ログを出力
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
         * @brief エラーログを出力
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