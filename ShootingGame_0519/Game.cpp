#include <chrono>
#include "Game.h"
#include "renderer.h"
#include "SceneManager.h"
#include "Application.h"
#include "Sound.h"
#include "TransitionManager.h"
#include "DebugUI.h"
#include "EffectManager.h"

void Game::GameInit()
{
    //カーソル非表示＆固定
    //Application::HideCursorAndClip(); 

    //DirectXのレンダラーを初期化
    Renderer::Init();
    
    Sound::Init();

    TransitionManager::Init();

    EffectManager::Init();

    SceneManager::Init(); //シーンマネージャーの初期化

    // デバッグUIの初期化
    DebugUI::Init(Renderer::GetDevice(), Renderer::GetDeviceContext());
}

void Game::GameUninit()
{
    // デバッグUIの終了処理
    DebugUI::DisposeUI();

    SceneManager::Uninit(); //シーンマネージャーの終了処理

    Sound::Uninit();
}

void Game::GameUpdate(float deltaTime)
{
    Sound::Update(deltaTime);

    SceneManager::Update(deltaTime); //シーンマネージャーの終了処理 

	EffectManager::Update(deltaTime);

    TransitionManager::Update(deltaTime);

}

void Game::GameDraw(float deltaTime)
{    
    //フレームの開始
    Renderer::Begin();
    //シーンマネージャーの描画処理
    SceneManager::Draw(deltaTime);
    EffectManager::Draw3D(deltaTime);
    TransitionManager::Draw(deltaTime);
    //フレームの終了
    Renderer::End();
}