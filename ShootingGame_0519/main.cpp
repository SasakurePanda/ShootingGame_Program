#include    "main.h"
#include    "Application.h"
#include <Windows.h>
#include <iostream>

static void ForceShowConsole()
{
    HWND consoleWindow = GetConsoleWindow();

    if (!consoleWindow)
    {
        AllocConsole();

        FILE* fp = nullptr;
        freopen_s(&fp, "CONOUT$", "w", stdout);
        freopen_s(&fp, "CONOUT$", "w", stderr);
        std::ios::sync_with_stdio(true);

        consoleWindow = GetConsoleWindow();
    }

    if (consoleWindow)
    {
        ShowWindow(consoleWindow, SW_SHOW);
        SetForegroundWindow(consoleWindow);
    }
}

int main(void)
{
#if defined(DEBUG) || defined(_DEBUG)
    ForceShowConsole();
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#else
    HWND consoleWindow = GetConsoleWindow();
    ShowWindow(consoleWindow, SW_HIDE);
#endif

    Application app(SCREEN_WIDTH, SCREEN_HEIGHT);
    app.Run();
    return 0;
}