#include <iostream>
#include "GameScene.h"
#include "Input.h"
#include "renderer.h"
#include "Application.h"

void GameScene::Init()
{
    // レンダラー初期化はアプリケーション側で済んでいる想定
    m_FreeCamera.Init();

    m_FreeCamera.SetPosition({ 0.0f, 0.0f, -5.0f });  // カメラ位置
    //m_FreeCamera.SetTarget({ 0.0f, 0.0f, 0.0f });    // 注視点

    // Model のロード
    m_Model = std::make_unique<Model>();
    if (!m_Model->LoadFromFile("Asset/Model/Cat/12221_Cat_v1_l3.obj"))
    {
        // 読み込み失敗時はログ出力など
        std::cout << "モデル読み込みに失敗しました\n";
        OutputDebugStringA("モデル読み込みに失敗しました：Assets/ship.obj\n");
    }
}

void GameScene::Update(uint64_t deltatime)
{
	Input::Update();
	m_FreeCamera.Update(deltatime);
}

void GameScene::Draw(uint64_t deltatime)
{
    // カメラ行列をシェーダーにセット
    Renderer::SetViewMatrix(m_FreeCamera.GetViewMatrix());
    Renderer::SetProjectionMatrix(m_FreeCamera.GetProjectionMatrix());

    SRT tr;
    tr.pos = { 0.0f, 0.0f, 0.0f };  // カメラの近くへ
    tr.rot = { XMConvertToRadians(90.0f), 0.0f, 0.0f };  // 回転なし
    tr.scale = { 0.25f, 0.25f, 0.25f };

    m_Model->Draw(tr);
}

void GameScene::Uninit()
{
    // 特に解放するものがあれば
    m_Model.reset();
}