#include <iostream>
#include <algorithm> 
#include "TitleScene.h"
#include "Input.h"
#include "Player.h"
#include "TitleBackGround.h"
#include "Input.h"
#include "TransitionManager.h"
#include "FreeCameraComponent.h"
#include "ModelComponent.h"
#include "TextureComponent.h"
#include "Sound.h"
#include "EffectManager.h"

void TitleScene::Init()
{
    m_camera = std::make_shared<CameraObject>();
    auto freeCamComp = std::make_shared<FreeCameraComponent>();
    m_camera->AddComponent(freeCamComp);

    m_Player = std::make_shared<GameObject>();
    m_Player->SetPosition(DirectX::SimpleMath::Vector3(0.0f, 0.0f, 40.0f));
    m_Player->SetScale(DirectX::SimpleMath::Vector3(1.5f, 1.5f, 1.5f));

    auto model = std::make_shared<ModelComponent>("Asset/Model/Player/Fighterjet.obj");
    model->SetColor(Color(1, 0, 0, 1));
    m_Player->AddComponent(model);

    m_TitleLogo = std::make_shared<GameObject>();
    auto LogoTexter = std::make_shared<TextureComponent>();
    LogoTexter->LoadTexture(L"Asset/UI/TitleLogo01.png");
    LogoTexter->SetSize(614.4f, 520.8f);
    LogoTexter->SetScreenPosition(340, -30);
    m_TitleLogo->AddComponent(LogoTexter);

    m_TitleText = std::make_shared<GameObject>();
    auto TextTexter = std::make_shared<TextureComponent>();
    TextTexter->LoadTexture(L"Asset/UI/TitleText02.png");
    TextTexter->SetSize(780, 260);
    TextTexter->SetScreenPosition(250, 370);
    m_TitleText->AddComponent(TextTexter);

    auto motion = std::make_shared<TitlePlayerMotionComponent>();

    m_Player->AddComponent(motion);
    
    m_TitleMotion = m_Player->GetComponent<TitlePlayerMotionComponent>();

    SetupPlayerPaths();
    ApplyCurrentPlayerPath();

    m_SkyDome = std::make_shared<SkyDome>("Asset/SkyDome/SkyDome_03.png");
	m_SkyDome->Initialize();

	AddObject(m_SkyDome);
	AddObject(m_camera);
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
    
    Sound::PlaySeWav(L"Asset/Sound/SE/TitlePlayerPassing.wav", 0.5f);
}

void TitleScene::Update(float deltatime)
{
    SetSceneObject();

    for (auto& obj : m_GameObjects)
    {
        if (obj)
        {
            obj->Update(deltatime);
        }
    }

    for (auto& obj : m_TextureObjects)
    {
        if (obj)
        {
            obj->Update(deltatime);
        }
    }

    //--------------最初の1回だけ：ロゴ表示＆入力解禁------------------
    if (!m_IsLogoShown && m_TitleMotion)
    {
        float safeDuration = max(0.01f, m_TitleMotion->GetDuration());
        float t = std::clamp(m_TitleMotion->GetTime() / safeDuration, 0.0f, 1.0f);

        if (t >= 0.7f)
        {
            Sound::PlayBgmWav(L"Asset/Sound/BGM/TitleBGM.wav", 0.1f);
            Sound::StopAllSe();

            m_IsLogoShown = true;

            AddTextureObject(m_TitleLogo);
            AddTextureObject(m_TitleText);

            if (m_TitleLogo)
            {
                m_TitleLogo->Initialize();
            }
            if (m_TitleText)
            {
                m_TitleText->Initialize();
            }
        }
    }

	//次のPlayerのベジェ曲線での動きへ進行
    if (m_TitleMotion)
    {
        float safeDuration = max(0.01f, m_TitleMotion->GetDuration());
        float t = std::clamp(m_TitleMotion->GetTime() / safeDuration, 0.0f, 1.0f);

        if (t >= 1.0f)
        {
            AdvancePlayerPath();
        }
    }

	//Logo表示後の点滅＆Enterキーでシーン遷移
    if (m_IsLogoShown)
    {
        m_BlinkTimer += deltatime;

        if (m_BlinkTimer >= m_BlinkInterval)
        {
            m_BlinkTimer -= m_BlinkInterval;
            m_BlinkVisible = !m_BlinkVisible;

            if (m_TitleText)
            {
                auto tex = m_TitleText->GetComponent<TextureComponent>();
                if (tex)
                {
                    tex->SetVisible(m_BlinkVisible);
                }
            }
        }

        if (Input::IsKeyDown(VK_RETURN))
        {
            if (TransitionManager::IsTransitioning()){ return; }

            Sound::PlaySeWav(L"Asset/Sound/SE/TitleSelect01.wav", 0.5f);
            
            TransitionManager::Start(3.0f,
                []()
                {
                    SceneManager::SetCurrentScene("GameScene");
                });
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
    auto freecam = m_camera->GetComponent<FreeCameraComponent>();

    if (freecam)
    {
        Renderer::SetViewMatrix(freecam->GetView());
        Renderer::SetProjectionMatrix(freecam->GetProj());
    }

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

/// <summary>
/// シーンのフレーム終了後のクリーンアップ処理関数
/// </summary>
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

/// <summary>
/// シーンのオブジェクトをセットする関数
/// </summary>
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

/// <summary>
/// ベジェ曲線で複数の動きを作るためにパスを設定する関数
/// </summary>
void TitleScene::SetupPlayerPaths()
{
    m_PlayerPaths.clear();

    //最初の動き(画面の奥から迫ってくる)
    BezierPath a;
    a.p0 = DirectX::SimpleMath::Vector3(-166.0f, 90.0f, -220.0f);
    a.p1 = DirectX::SimpleMath::Vector3(-70.0f, 12.0f, -80.0f);
    a.p2 = DirectX::SimpleMath::Vector3(10.0f, -6.0f, -3.0f);  // 近くを横切る
    a.p3 = DirectX::SimpleMath::Vector3(45.0f, -14.0f, 55.0f);  // 背面へ
    a.duration = 2.2f;
    m_PlayerPaths.push_back(a);

	//高い所からゆっくり横切る
    BezierPath b;
    b.p0 = DirectX::SimpleMath::Vector3(42.0f, 20.0f, -170.0f);
    b.p1 = DirectX::SimpleMath::Vector3(26.0f, 16.0f, -95.0f);
    b.p2 = DirectX::SimpleMath::Vector3(20.0f, 5.0f, -16.0f);
    b.p3 = DirectX::SimpleMath::Vector3(-28.0f, -12.0f, 40.0f);
    b.duration = 2.0f;
    m_PlayerPaths.push_back(b);

    //低い所からスッと横切る
    BezierPath c;
    c.p0 = DirectX::SimpleMath::Vector3(-166.0f, -6.0f, -150.0f);
    c.p1 = DirectX::SimpleMath::Vector3(-25.0f, -4.0f, -85.0f);
    c.p2 = DirectX::SimpleMath::Vector3( -4.0f, -2.0f, -12.0f);
    c.p3 = DirectX::SimpleMath::Vector3( 18.0f, -6.0f, 30.0f);
    c.duration = 1.6f;
    m_PlayerPaths.push_back(c);

    m_CurrentPathIndex = 0;
}

/// <summary>
/// 現在のパスを適応するための関数
/// </summary>
void TitleScene::ApplyCurrentPlayerPath()
{
    if (!m_TitleMotion) { return; }
    if (m_PlayerPaths.empty()) { return; }

    const BezierPath& path = m_PlayerPaths[m_CurrentPathIndex];
    m_TitleMotion->SetControlPoints(path.p0, path.p1, path.p2, path.p3);
    m_TitleMotion->SetDuration(path.duration);
    m_TitleMotion->ResetTime();
}

/// <summary>
/// ベジェ曲線用を次のパスに進める関数
/// </summary>
void TitleScene::AdvancePlayerPath()
{
    if (m_PlayerPaths.empty()) { return; }

    m_CurrentPathIndex++;

    if (m_CurrentPathIndex >= static_cast<int>(m_PlayerPaths.size()))
    {
        if (m_LoopPaths)
        {
            m_CurrentPathIndex = 0;
        }
        else
        {
            m_CurrentPathIndex = static_cast<int>(m_PlayerPaths.size()) - 1;
        }
    }

    ApplyCurrentPlayerPath();
}

