#include <Windows.h>
#include <exception>
#include "Athena/Utils/Logger.h"
#include "Athena/Core/Device.h"

/**
 * @brief Windowsアプリケーションのエントリーポイント
 *
 * @param hInstance 現在のアプリケーションインスタンスのハンドル
 * @param hPrevInstance 以前のインスタンス（Win32では常にNULL）
 * @param lpCmdLine コマンドライン引数
 * @param nCmdShow ウィンドウの表示方法（最小化、最大化など）
 */
int WINAPI WinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPSTR lpCmdLine,
    _In_ int nCmdShow)
{
    // 未使用パラメータの警告を抑制
    UNREFERENCED_PARAMETER(hInstance);
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);
    UNREFERENCED_PARAMETER(nCmdShow);

    try {
        // ログシステム初期化
        Athena::Logger::Initialize();
        Athena::Logger::Info("Athena Renderer starting...");

        // デバイス初期化のテスト
        Athena::Device device;
        device.Initialize(true);  // デバッグレイヤー有効

        Athena::Logger::Info("Device initialized successfully!");
        Athena::Logger::Info("Press OK to exit...");

        // ユーザーの確認待ち（テスト用）
        MessageBoxA(
            NULL,
            "Athena Renderer initialized successfully!\n"
            "Check Visual Studio Output window for details.",
            "Success",
            MB_OK | MB_ICONINFORMATION
        );

        // 終了処理
        device.Shutdown();
        Athena::Logger::Shutdown();

        return 0;
    }
    catch (const std::exception& e) {
        // エラー発生時
        Athena::Logger::Error("Fatal error: %s", e.what());

        MessageBoxA(
            NULL,
            e.what(),
            "Error",
            MB_OK | MB_ICONERROR
        );

        return 1;
    }
}