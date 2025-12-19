#include <iostream>
#include <algorithm> 
#include "TitleScene.h"
#include "Input.h"
#include "Player.h"
#include "TitleBackGround.h"
#include "Input.h"
#include "TransitionManager.h"
#include "CameraObject.h"
#include "FreeCameraComponent.h"
#include "ModelComponent.h"
#include "TextureComponent.h"

void TitleScene::Init()
{
    auto cameraObj = std::make_shared<CameraObject>();
    auto freeCamComp = std::make_shared<FreeCameraComponent>();
	cameraObj->AddComponent(freeCamComp);

    m_Player = std::make_shared<GameObject>();
    m_Player->SetPosition(DirectX::SimpleMath::Vector3(0.0f, 0.0f, 40.0f));
    m_Player->SetScale(DirectX::SimpleMath::Vector3(1.5f, 1.5f, 1.5f));
    //TestOJ->SetRotation(Vector3(0.0f, DirectX::XM_PI, 0.0f));
    
    auto model = std::make_shared<ModelComponent>("Asset/Model/Player/Fighterjet.obj");
    model->SetColor(Color(1, 0, 0, 1));
    m_Player->AddComponent(model);

    m_TitleLogo = std::make_shared<GameObject>();
    auto LogoTexter = std::make_shared<TextureComponent>();
    LogoTexter->LoadTexture(L"Asset/UI/TitleLogo01.png");
    LogoTexter->SetSize(614.4, 460.8);
    LogoTexter->SetScreenPosition(360, -30);
    m_TitleLogo->AddComponent(LogoTexter);
    

    m_TitleText = std::make_shared<GameObject>();
    auto TextTexter = std::make_shared<TextureComponent>();
    TextTexter->LoadTexture(L"Asset/UI/TitleText01.png");
    TextTexter->SetSize(307.2, 230.4);
    TextTexter->SetScreenPosition(520, 400);
    m_TitleText->AddComponent(TextTexter);

    //AddTextureObject(TitleLogo);
    //AddTextureObject(TitleText);

    auto motion = std::make_shared<TitlePlayerMotionComponent>();
    if (motion)
    {
        motion->SetDuration(2.7f); 
        motion->SetControlPoints(
            Vector3(-166.0f, 90.0f,-220.0f),
            Vector3(-70.0f,  12.0f, -80.0f),
            Vector3( 10.0f, -6.0f, -3.0f),
            Vector3( 45.0f, -14.0f, 55.0f));
    }
    m_Player->AddComponent(motion);

    m_TitleMotion = m_Player->GetComponent<TitlePlayerMotionComponent>();

    m_SkyDome = std::make_shared<SkyDome>("Asset/SkyDome/SkyDome_03.png");
	m_SkyDome->Initialize();

	AddObject(m_SkyDome);
	AddObject(cameraObj);
    //cameraObj->SetCameraComponent(freeCamComp);
    AddObject(m_Player);

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
    
    //m_Player->SetRotation(Vector3(0.0f, 0.0f, 0.0f));
}

void TitleScene::Update(float deltatime)
{
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
	}

    for (auto& obj : m_TextureObjects)
    {
        if (obj) obj->Update(deltatime);
        Vector3 pos = obj->GetPosition();
    }

    if (!m_IsLogoShown && m_TitleMotion)
    {
        float safeDuration = max(0.01f, m_TitleMotion->GetDuration());
        float t = std::clamp(m_TitleMotion->GetTime() / safeDuration, 0.0f, 1.0f);

        if (t >= 0.7f)
        {
            m_IsLogoShown = true;
            AddTextureObject(m_TitleLogo);
            AddTextureObject(m_TitleText);

            // 追加したなら初期化が必要な設計ならここ
            if (m_TitleLogo) { m_TitleLogo->Initialize(); }
        }
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
