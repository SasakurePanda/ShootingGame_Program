#include <iostream>
#include "TitleScene.h"
#include "Input.h"
#include "Player.h"
#include "TitleBackGround.h"
#include "Input.h"
#include "TransitionManager.h"
#include "CameraObject.h"
#include "FreeCameraComponent.h"
#include "TitlePlayerMotionComponent.h"
#include "ModelComponent.h"

void TitleScene::Init()
{
    auto cameraObj = std::make_shared<CameraObject>();
    auto freeCamComp = std::make_shared<FreeCameraComponent>();
	cameraObj->AddComponent(freeCamComp);

	auto TestOJ = std::make_shared<GameObject>();
    TestOJ->SetPosition(DirectX::SimpleMath::Vector3(0.0f, 0.0f, 40.0f));
    TestOJ->SetScale(DirectX::SimpleMath::Vector3(0.3f, 0.3f, 0.3f));
    
    auto model = std::make_shared<ModelComponent>("Asset/Model/Robot/12211_Robot_l2.obj");
    model->SetColor(Color(1, 0, 0, 1));
    TestOJ->AddComponent(model);

    auto motion = std::make_shared<TitlePlayerMotionComponent>();
    if (motion)
    {
        motion->SetDuration(2.0f); // 3秒ちょいで横切る
        motion->SetControlPoints(
            Vector3(-35.0f, 8.0f, 140.0f),
            Vector3(-18.0f, 14.0f, 95.0f),
            Vector3(22.0f, 6.0f, 62.0f),
            Vector3(45.0f, -2.0f, 15.0f)
        );
    }
    TestOJ->AddComponent(motion);

    m_SkyDome = std::make_shared<SkyDome>("Asset/SkyDome/SkyDome_03.png");
	m_SkyDome->Initialize();

	AddObject(m_SkyDome);
	AddObject(cameraObj);
    cameraObj->SetCameraComponent(freeCamComp);
    AddObject(TestOJ);

    SetSceneObject();

    for (auto& obj : m_GameObjects)
    {
        if (obj)
        {
            obj->Initialize();
        }
    }
    
    if (m_SkyDome && freeCamComp)
    {
        m_SkyDome->SetCamera(freeCamComp.get());
    }
    
    TestOJ->SetRotation(Vector3(80.0f, 0.0f, 0.0f));
}

void TitleScene::Update(float deltatime)
{
    //Input::Update();

    SetSceneObject();

    if (Input::IsKeyDown(VK_SPACE))
    {
        //旧式はこっち
        SceneManager::SetCurrentScene("GameScene");
    }

    for (auto& obj : m_GameObjects)
    {
        if (obj) obj->Update(deltatime);
        Vector3 pos = obj->GetPosition();
		//std::cout << "Object Position: (" << pos.x << ", " << pos.y << ", " << pos.z << ")\n";
    }
}

void TitleScene::Draw(float deltatime)
{
    for (auto& obj : m_GameObjects)
    {
        if (obj) obj->Draw(deltatime);
    }
}

void TitleScene::Uninit()
{
    
}
    

void TitleScene::AddObject(std::shared_ptr<GameObject> obj)
{
    if (!obj) return;
    // シーン参照を GameObject に教えておく（後述）
    obj->SetScene(this);
    m_AddObjects.push_back(obj);
}


void TitleScene::RemoveObject(std::shared_ptr<GameObject> obj)
{
    m_DeleteObjects.push_back(obj);
}

void TitleScene::RemoveObject(GameObject* obj)
{
    //if (!obj) return;
    //// 重複追加を防ぎたい場合チェックしてから push_back してもよい
    //m_DeleteObjects.push_back(obj);
}

void TitleScene::FinishFrameCleanup()
{
    for (auto& p : m_DeleteObjects) // p は shared_ptr<GameObject>
    {
        auto it = std::find_if(m_GameObjects.begin(), m_GameObjects.end(), [&](const std::shared_ptr<GameObject>& sp)
            {return sp == p; });
        if (it != m_GameObjects.end()) m_GameObjects.erase(it);
    }
    m_DeleteObjects.clear();
}

void TitleScene::SetSceneObject()
{
    if (!m_AddObjects.empty())
    {
        // 一括追加（file-safe: reserve してから insert）
        m_GameObjects.reserve(m_GameObjects.size() + m_AddObjects.size());
        m_GameObjects.insert(m_GameObjects.end(),
            std::make_move_iterator(m_AddObjects.begin()),
            std::make_move_iterator(m_AddObjects.end()));
        m_AddObjects.clear();
    }
}
