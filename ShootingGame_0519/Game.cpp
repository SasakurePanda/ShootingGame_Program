#include <chrono>
#include "Game.h"
#include "renderer.h"
#include "SceneManager.h"
#include "Application.h"
#include "TransitionManager.h"
#include "DebugUI.h"

void Game::GameInit()
{
    //カーソル非表示＆固定
    //Application::HideCursorAndClip(); 

    //DirectXのレンダラーを初期化
    Renderer::Init();

    TransitionManager::Init();

    SceneManager::Init(); //シーンマネージャーの初期化

    // デバッグUIの初期化
    DebugUI::Init(Renderer::GetDevice(), Renderer::GetDeviceContext());

}

void Game::GameUninit()
{
    // デバッグUIの終了処理
    DebugUI::DisposeUI();

    SceneManager::Uninit(); //シーンマネージャーの終了処理
}

void Game::GameUpdate(float deltaTime)
{
    SceneManager::Update(deltaTime); //シーンマネージャーの終了処理
}

void Game::GameDraw(float deltaTime)
{    
    //フレームの開始
    Renderer::Begin();

    SceneManager::Draw(deltaTime); //シーンマネージャーの描画処理

    // デバッグUIの描画
    DebugUI::Render();

    //フレームの終了
    Renderer::End();
}