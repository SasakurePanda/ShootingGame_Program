#include "Application.h"
#include "renderer.h"
#include <chrono>

constexpr auto ClassName  = TEXT("2025 就職作品 中江文瞳");         //ウィンドウクラス名.
constexpr auto WindowName = TEXT("2025 就職作品 中江文瞳");        //ウィンドウ名.

//-----------------------------------------------------------------------------
// Class Static
//-----------------------------------------------------------------------------
HINSTANCE  Application::m_hInst;        //インスタンスハンドルです.
HWND       Application::m_hWnd;         //ウィンドウハンドルです.
uint32_t   Application::m_Width;        //ウィンドウの横幅です.
uint32_t   Application::m_Height;       //ウィンドウの縦幅です.

//ImGuiのWin32プロシージャハンドラ(マウス対応)
//extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

void Application::Run()
{
    if (InitApp())      //Application関連のInitを実行し、成功した場合
    {
        MainLoop();     //メインループを実行
    }

    UninitApp();        //狩猟処理
}

bool Application::InitApp()
{
    // ウィンドウの初期化.
    if (!InitWnd())
    {
        return false;
    }

    // 正常終了.
    return true;
}

void Application::UninitApp()
{
    Game::GameUninit(); // ゲームの後処理
    Renderer::Uninit(); // DirectXのリソース解放
    UninitWnd(); // ウィンドウの後処理
}

void Application::MainLoop()
{
    MSG msg = {};

    // ゲームの初期化
    Game::GameInit();

    // メインループ
    auto previousTime = std::chrono::steady_clock::now();
    while (msg.message != WM_QUIT)
    {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else
        {
            // ΔTime計算
            auto currentTime = std::chrono::steady_clock::now();
            uint64_t deltaTime = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - previousTime).count();
            previousTime = currentTime;

            // 毎フレームの更新・描画
            Game::GameUpdate(deltaTime);
            Game::GameDraw(deltaTime);
        }
    }

    // ゲーム終了処理
    Game::GameUninit();
}

bool Application::InitWnd()
{
    // インスタンスハンドルを取得.
    auto hInst = GetModuleHandle(nullptr);
    if (hInst == nullptr)
    {
        return false;
    }

    // ウィンドウの設定.
    WNDCLASSEX wc = {};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hIcon = LoadIcon(hInst, IDI_APPLICATION);  //エグゼのアイコンの見た目を変えられる
    wc.hCursor = LoadCursor(hInst, IDC_ARROW);    //カーソルの見た目を変えられる
    //wcex.hCursor =		   //NULL,カーソルの画像パス,
    //	(HCURSOR)LoadImage(NULL, "path_to_cursor_file.cur", 
    //					   //ロードする画像の種類,幅の設定(0はデフォ),ロード方法
    //					   IMAGE_CURSOR, 0, 0, LR_LOADFROMFILE);
    wc.hbrBackground = GetSysColorBrush(COLOR_BACKGROUND);  //ウィンドウの初期背景カラー
    wc.lpszMenuName = nullptr;
    wc.lpszClassName = ClassName;
    wc.hIconSm = LoadIcon(hInst, IDI_APPLICATION);

    // ウィンドウの登録.
    if (!RegisterClassEx(&wc))
    {
        return false;
    }

    // インスタンスハンドル設定.
    m_hInst = hInst;

    // ウィンドウのサイズを設定.
    RECT rc = {};
    rc.right = static_cast<LONG>(m_Width);
    rc.bottom = static_cast<LONG>(m_Height);

    // ウィンドウサイズを調整.
    auto style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU;
    AdjustWindowRect(&rc, style, FALSE);

    // ウィンドウを生成.
    m_hWnd = CreateWindowEx(
        0,
        ClassName,
        WindowName,
        style,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        rc.right - rc.left,
        rc.bottom - rc.top,
        nullptr,
        nullptr,
        m_hInst,
        nullptr);

    if (m_hWnd == nullptr)
    {
        return false;
    }

    // ウィンドウを表示.
    ShowWindow(m_hWnd, SW_SHOWNORMAL);

    // ウィンドウを更新.
    UpdateWindow(m_hWnd);

    // ウィンドウにフォーカスを設定.
    SetFocus(m_hWnd);

    // 正常終了.
    return true;
}

void Application::UninitWnd()
{
    if (m_hInst != nullptr)//ウィンドウが登録されている場合
    {
        UnregisterClass(ClassName, m_hInst);//ウィンドウの登録を解除
    }

    //解放ミスを防ぐためにnullptrを入れておく
    m_hInst = nullptr;      //インスタンスハンドルにnullptrを入れる
    m_hWnd = nullptr;       //ウィンドウハンドルにnullptrを入れる
}

// ウィンドウプロシージャ
LRESULT CALLBACK Application::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_DESTROY:       //ウィンドウが消える時
        PostQuitMessage(0);
        break;
    }
    return DefWindowProc(hWnd, message, wParam, lParam);
}
