#pragma once
#pragma comment(lib, "winmm.lib")
#include <Windows.h>
#include <cstdint>
#include "Game.h"

class Application
{
public:
    Application(uint32_t width, uint32_t height) //コンストラクタ
    {
        m_Height = height;  //縦幅
        m_Width = width;    //横幅

        timeBeginPeriod(1);//
    }

	~Application()      //デストラクタ
    {
        timeEndPeriod(1);//タイマーを通常に戻す
    }

	void Run();       //アプリケーション全体の行動

    static uint32_t GetWidth()//横幅を取得する
    {
        return m_Width;
    }

    static uint32_t GetHeight()//縦幅を取得する
    {
        return m_Height;
    }

    static HWND GetWindow()//ウィンドウハンドルを取得
    {
        return m_hWnd;
    }

    static HINSTANCE GetHInstance()//インスタンスハンドルを取得
    {
        return m_hInst;
    }

private:
    static HINSTANCE   m_hInst;    //インスタンスハンドル
    static HWND        m_hWnd;     //ウィンドウハンドル
    static uint32_t    m_Width;    //ウィンドウの横幅
    static uint32_t    m_Height;   //ウィンドウの縦幅 
    
    static bool InitApp();   //アプリケーションの初期化
    static void UninitApp(); //アプリケーションの終了処理
    static bool InitWnd();   //ウィンドウの初期化
    static void UninitWnd(); //ウィンドウの終了処理
    static void MainLoop();  //ゲームのメインループ

    //ウィンドウプロシージャ
    static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp);
};