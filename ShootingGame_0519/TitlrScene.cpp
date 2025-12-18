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
#include "TextureComponent.h"

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

    auto TitleLogo = std::make_shared<GameObject>();
    auto LogoTexter = std::make_shared<TextureComponent>();
    LogoTexter->LoadTexture(L"Asset/UI/TitleLogo01.png");
    LogoTexter->SetSize(768, 576);
    LogoTexter->SetScreenPosition(420, 30);
    TitleLogo->AddComponent(LogoTexter);
    

    auto TitleText = std::make_shared<GameObject>();
    auto TextTexter = std::make_shared<TextureComponent>();
    TextTexter->LoadTexture(L"Asset/UI/TitleText01.png");
    LogoTexter->SetSize(768, 576);
    TitleText->AddComponent(TextTexter);

    AddTextureObject(TitleLogo);
    AddTextureObject(TitleText);

    auto motion = std::make_shared<TitlePlayerMotionComponent>();
    if (motion)
    {
        motion->SetDuration(2.0f); 
        motion->SetControlPoints(
            Vector3(-170.0f, 84.0f,-220.0f),
            Vector3(-40.0f,  2.0f, -120.0f),
            Vector3( 10.0f, -6.0f, -3.0f),
            Vector3( 45.0f, -14.0f, 55.0f));
    }
    TestOJ->AddComponent(motion);

    m_SkyDome = std::make_shared<SkyDome>("Asset/SkyDome/SkyDome_03.png");
	m_SkyDome->Initialize();

	AddObject(m_SkyDome);
	AddObject(cameraObj);
    //cameraObj->SetCameraComponent(freeCamComp);
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

void TitleScene::Draw(float dt)
{
    DrawWorld(dt);
    DrawUI(dt);
}

void TitleScene::DrawWorld(float deltatime)
{
    for (auto& obj : m_GameObjects)
    {
        if (!obj) { continue; }
        obj->Draw(deltatime);
    }
}

void TitleScene::DrawUI(float deltatime)
{
    for (auto& obj : m_TextureObjects)
    {
        if (!obj) { continue; }
        obj->Draw(deltatime);
    }
}

void TitleScene::Uninit()
{
    
}
    

void TitleScene::AddObject(std::shared_ptr<GameObject> obj)
{
    if (!obj)
    {
        return;
    }

    //既にシーン内にいるかpendingにいるかチェック
    auto itInScene = std::find_if(m_GameObjects.begin(), m_GameObjects.end(),
        [&](const std::shared_ptr<GameObject>& sp) { return sp.get() == obj.get(); });

    //既に実体がある
    if (itInScene != m_GameObjects.end())
    {
        return;
    }

    auto itPending = std::find_if(m_AddObjects.begin(), m_AddObjects.end(),
        [&](const std::shared_ptr<GameObject>& sp) { return sp.get() == obj.get(); });

    // 追加予定にすでにある
    if (itPending != m_AddObjects.end())
    {
        return;
    }

    //所属しているSceneを登録
    obj->SetScene(this);

    //実際に配列にプッシュする
    m_AddObjects.push_back(obj);
}

void TitleScene::AddTextureObject(std::shared_ptr<GameObject> obj)
{
    if (!obj)
    {
        return;
    }

    //既にシーン内にいるかpendingにいるかチェック
    auto itInScene = std::find_if(m_GameObjects.begin(), m_GameObjects.end(),
        [&](const std::shared_ptr<GameObject>& sp) { return sp.get() == obj.get(); });

    //既に実体がある
    if (itInScene != m_GameObjects.end())
    {
        return;
    }

    //所属しているSceneを登録
    obj->SetScene(this);

    //実際に配列にプッシュする
    m_TextureObjects.push_back(obj);
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
