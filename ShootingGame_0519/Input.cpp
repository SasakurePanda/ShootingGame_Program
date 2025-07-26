#include "Input.h"
#include "Application.h"

BYTE Input::m_CurrentKeys[256];
BYTE Input::m_PreviousKeys[256];

POINT Input::m_CurrentMousePos;   //現在のフレームのマウスの座標
POINT Input::m_PreviousMousePos;  //前のフレームのマウスの座標

BYTE Input::m_CurrentMouseButtons[3];   //現在のフレームのマウスの座標
BYTE Input::m_PreviousMouseButtons[3];  //前のフレームのマウスの座標

float Input::m_MouseSensitivity = 0.005f;   //マウス感度

void Input::Update()
{
    // キー
    memcpy(m_PreviousKeys, m_CurrentKeys, sizeof(m_CurrentKeys));
    // マウスボタン
    memcpy(m_PreviousMouseButtons, m_CurrentMouseButtons, sizeof(m_CurrentMouseButtons));
    // マウス座標
    m_PreviousMousePos = m_CurrentMousePos;

    //現在のキー状態を取得
    if (!GetKeyboardState(m_CurrentKeys))
    {
        // エラー処理、ログ出力など
        OutputDebugStringA("キーが取れていません！！\n");
    }


    //現在のフレームのマウスの座標を取得
    if (!GetCursorPos(&m_CurrentMousePos))
    {
        OutputDebugStringA("GetCursorPos() Failed\n");
    }
    else
    {
        // ウィンドウ左上を (0,0) とするクライアント座標に変換
        ScreenToClient(Application::GetWindow(), &m_CurrentMousePos);
    }


    m_CurrentMouseButtons[0] = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) ? 1 : 0;
    m_CurrentMouseButtons[1] = (GetAsyncKeyState(VK_RBUTTON) & 0x8000) ? 1 : 0;
    m_CurrentMouseButtons[2] = (GetAsyncKeyState(VK_MBUTTON) & 0x8000) ? 1 : 0;
}

bool Input::IsKeyDown(unsigned char key)
{
    return (m_CurrentKeys[key] & 0x80) != 0;
}

// Input.cpp
bool Input::IsKeyPressed(int keyCode)
{
    return (GetAsyncKeyState(keyCode) & 0x8000) != 0;
}

POINT Input::GetMouseDelta()
{
    POINT delta;
    //カメラのX座標の移動量を差で計算する
    delta.x = m_CurrentMousePos.x - m_PreviousMousePos.x;
    //カメラのY座標の移動量を差で計算する
    delta.y = m_CurrentMousePos.y - m_PreviousMousePos.y;
    return delta;
}

POINT Input::GetMousePosition()
{
    return m_CurrentMousePos;
}

bool Input::IsMouseRightDown()
{
    return m_CurrentMouseButtons[1] != 0;
}

bool Input::IsMouseRightPressed()
{
    return m_CurrentMouseButtons[1] != 0 && m_PreviousMouseButtons[1] == 0;
}

bool Input::IsMouseLeftDown()
{
    return m_CurrentMouseButtons[0] != 0;
}

bool Input::IsMouseLeftPressed()
{
    return m_CurrentMouseButtons[0] != 0 && m_PreviousMouseButtons[0] == 0;
}