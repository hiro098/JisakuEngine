#include <Windows.h>
#include "app/App.h"
#include <spdlog/spdlog.h>

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    // ログ設定
    spdlog::set_level(spdlog::level::debug);
    spdlog::info("JisakuEngine starting...");

    try
    {
        jisaku::App app;
        if (!app.Initialize(hInstance))
        {
            spdlog::error("Failed to initialize application");
            return -1;
        }

        return app.Run();
    }
    catch (const std::exception& e)
    {
        spdlog::error("Exception: {}", e.what());
        return -1;
    }
}
