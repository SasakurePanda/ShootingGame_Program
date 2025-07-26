#include <chrono>
#include "Game.h"
#include "renderer.h"
#include "SceneManager.h"

void Game::GameInit()
{
    // DirectXのレンダラーを初期化
    Renderer::Init();

    SceneManager::Init(); // シーンマネージャーの初期化
}

void Game::GameUninit()
{
    SceneManager::Uninit(); // シーンマネージャーの終了処理
}

void Game::GameUpdate(uint64_t deltaTime)
{
    SceneManager::Update(deltaTime); // シーンマネージャーの終了処理
}

void Game::GameDraw(uint64_t deltaTime)
{    
    // フレームの開始
    Renderer::Begin();

    SceneManager::Draw(deltaTime); // シーンマネージャーの描画処理

    // フレームの終了
    Renderer::End();
}

void Game::GameLoop()
{
    using Clock = std::chrono::steady_clock; //現在の時間を取得
    auto previousTime = Clock::now();        //一定のペースで時間を測る

    while (true)
    {
        auto currentTime = Clock::now();
        uint64_t deltaTime = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - previousTime).count();
        previousTime = currentTime;

        GameUpdate(deltaTime); // ゲームの更新処理
        GameDraw(deltaTime);   // ゲームの描画処理
    }
}