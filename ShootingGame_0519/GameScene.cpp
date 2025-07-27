#include <iostream>
#include "GameScene.h"
#include "Input.h"
#include "renderer.h"
#include "Application.h"

void GameScene::Init()
{
    //プレイヤー作成
    m_player = std::make_shared<Player>();
    m_player -> SetPosition({ 0.0f, 0.0f, 0.0f });
    m_player -> SetRotation({ 0.0f,0.0f, 0.0f });
    m_player -> SetScale({ 0.3f, 0.3f, 0.3f });
    m_player -> Initialize();
    auto moveComp = m_player->GetComponent<MoveComponent>();
    
    // ShootingComponent に this（現在のシーン）を渡す
    auto shootComp = m_player->GetComponent<ShootingComponent>();
    if (shootComp)
    {
        shootComp->SetScene(this);
    }

    //CameraObjectst作成
    m_FollowCamera = std::make_shared<CameraObject>();
    m_FollowCamera ->Initialize();
    auto cameraComp = m_FollowCamera->GetComponent<FollowCameraComponent>();

    if (moveComp && cameraComp)
    {
        moveComp->SetCameraView(cameraComp.get()); // ←重要！
    }

    m_GridFloor = std::make_shared<GridFloor>();
    m_GridFloor->SetPosition({ 0.0f, 0.0f, 40.0f }); // プレイヤーの下に置く
    m_GridFloor->SetScale({ 0.1f, 0.1f, 0.1f });    // 広くする（必要に応じて）
    m_GridFloor->Initialize();

    m_FollowCamera->GetCameraComponent()->SetTarget(m_player.get());

    m_GameObjects.push_back(m_player);
    m_GameObjects.push_back(m_FollowCamera);
    m_GameObjects.push_back(m_GridFloor);
}

void GameScene::Update(uint64_t deltatime)
{
    Input::Update(); // Inputの更新

    for (size_t i = 0; i < m_GameObjects.size(); ++i)
    {
        if (!m_GameObjects[i])
        {
            std::cout << "GameObject[" << i << "] is nullptr!\n";
            continue;
        }

        m_GameObjects[i]->Update(); // null じゃなければ更新
    }

    Vector3 player = m_player->GetPosition();
    std::cout << "プレイヤーの位置：" << player.x << "," << player.y << "," << player.z << "\n";
}

void GameScene::Draw(uint64_t deltatime)
{
    for (auto& obj : m_GameObjects)
    {
        obj->Draw();
    }
}

void GameScene::Uninit()
{
    m_player.reset();
}

void GameScene::AddObject(std::shared_ptr<GameObject> obj)
{
    m_GameObjects.push_back(obj); // m_GameObjects は std::vector<std::shared_ptr<GameObject>>
}