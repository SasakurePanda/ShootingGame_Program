#include <iostream>
#include "GameScene.h"
#include "Input.h"
#include "DebugGlobals.h" 
#include "renderer.h"
#include "Application.h"
#include "Collision.h"
#include "CollisionManager.h"
#include "SkyDome.h"
#include "HPBar.h"
#include "Building.h"
#include "PatrolComponent.h"
#include "CircularPatrolComponent.h"

void GameScene::DebugCollisionMode()
{
    static int selected = 0;

    ImGui::Begin("Collision Debug Mode Select");

    ImGui::RadioButton("OnDebug Mode", &selected, 0);
    ImGui::RadioButton("NoDebug Mode", &selected, 1);

    ImGui::End();

    if (selected == 0)
    {
        isCollisionDebugMode = true;
    }
    else
    {
        isCollisionDebugMode = false;
    }
}

void GameScene::DebugSetPlayerSpeed()
{
    ImGui::Begin("DebugPlayerSpeed");

    static float speed = 10.0f;
    
    ImGui::SliderFloat("PlayerSpeed", &speed, 0.1f, 35.0f);

    setSpeed = speed;

    ImGui::End();
}

void GameScene::Init()
{    
    // デバッグMODE SELECT
    DebugUI::RedistDebugFunction([this]() {DebugCollisionMode();});

    DebugUI::RedistDebugFunction([this]() {DebugSetPlayerSpeed();});

        // 既存 Init の先頭あたりで一度だけ初期化
    m_debugRenderer = std::make_unique<DebugRenderer>();
    m_debugRenderer->Initialize(Renderer::GetDevice(), Renderer::GetDeviceContext(),
        L"DebugLineVS.cso", L"DebugLinePS.cso");

    //-----------------------スカイドーム作成-------------------------------
    m_SkyDome = std::make_shared<SkyDome>("Asset/SkyDome/SkyDome_02.png");
    m_SkyDome->Initialize();

    //--------------------------プレイヤー作成---------------------------------
    m_player = std::make_shared<Player>();
    m_player->SetPosition({ 0.0f, 0.0f, 0.0f });
    m_player->SetRotation({ 0.0f,0.0f,0.0f });
    m_player->SetScale({ 0.3f, 0.3f, 0.3f });
    m_player->Initialize();
    auto moveComp = m_player->GetComponent<MoveComponent>();

    //--------------------------エネミー作成---------------------------------
    auto enemy = std::make_shared<Enemy>();
    enemy->SetScene(this);
    enemy->SetPosition({ 10.0f, 0.0f, 0.0f });
    enemy->SetInitialHP(3);

    //モデルの読み込み（失敗時に備えログなども可）
    auto model = std::make_shared<ModelComponent>();
    model->LoadModel("Asset/Model/Robot/uploads_files_3862208_Cube.fbx");
    enemy->AddComponent(model);

    auto col = std::make_shared<OBBColliderComponent>();
    col->SetSize({ 2.0f,2.0f,2.0f });
    enemy->AddComponent(col);

    // 巡回コンポーネントを追加
    auto patrol = std::make_shared<PatrolComponent>();
    std::vector<DirectX::SimpleMath::Vector3> pts = 
    {
        { 0.0f,  0.0f, 0.0f },
        { 125.0f, 0.0f, 0.0f },
        { 125.0f, 0.0f, 125.0f },
        { 0.0f,  0.0f, 125.0f }
    };
    patrol->SetWaypoints(pts);
    patrol->SetSpeed(25.0f);
    patrol->SetPingPong(true); // 往復
    patrol->SetArrivalThreshold(0.4f);
    enemy->AddComponent(patrol);

    AddObject(enemy); // シーン側に追加

    auto circEnemy = std::make_shared<Enemy>(); // あなたの Enemy ベースを使うならそちらで
    circEnemy->SetScene(this);
    circEnemy->SetPosition({ 8.0f, 0.0f, 0.0f });

    // モデル・コライダ等
    model = std::make_shared<ModelComponent>();
    model->LoadModel("Asset/Model/Robot/uploads_files_3862208_Cube.fbx");
    circEnemy->AddComponent(model);

    col = std::make_shared<OBBColliderComponent>();
    col->SetSize({ 2.0f,2.0f,2.0f });
    circEnemy->AddComponent(col);

    // 円周移動コンポーネント
    auto circ = std::make_shared<CirculPatrolComponent>();
    circ->SetCenter({ 0.0f, 0.0f, 0.0f });
    circ->SetRadius(8.0f);
    circ->SetAngularSpeed(DirectX::XM_PI / 2.0f); // 90deg/s
    circ->SetClockwise(true);
    circEnemy->AddComponent(circ);

    AddObject(circEnemy);

    //-------------------------レティクル作成-------------------------------------
    m_reticle = std::make_shared<Reticle>(L"Asset/UI/26692699.png", m_reticleW);
    RECT rc{};
    GetClientRect(Application::GetWindow(), &rc);
    float x = (rc.right - rc.left) / 2;
    float y = (rc.bottom - rc.top) / 2;
    m_reticle->Initialize();
    
    auto HPbar = std::make_shared<TitleBackGround>(L"Asset/UI/HPBar.png", 1280); 
    HPbar->Initialize();

    //------------------------------追尾カメラ作成---------------------------------
    m_FollowCamera = std::make_shared<CameraObject>();
    m_FollowCamera->Initialize();
    
    auto shootComp = m_player->GetComponent<ShootingComponent>();
    //ShootingComponent に this（現在のシーン）を渡す
    if (shootComp)
    {
        shootComp->SetScene(this);
    }
   
    auto cameraComp = m_FollowCamera->GetComponent<FollowCameraComponent>();

    if (moveComp && cameraComp)
    {
        moveComp->SetCameraView(cameraComp.get()); 
    }

    if (shootComp && cameraComp)
    {
        shootComp->SetScene(this);
        shootComp->SetCamera(cameraComp.get());
    }

    if (cameraComp)
    {
        m_SkyDome->SetCamera(cameraComp.get()); //ICameraViewProvider* を受け取る場合
    }

    m_GameObjects.insert(m_GameObjects.begin(), m_SkyDome);

    m_FollowCamera->GetCameraComponent()->SetTarget(m_player.get());

    //AddTextureObject(HPbar);
    AddTextureObject(m_reticle);

    AddObject(m_player);
    //AddObject(m_Building);
    AddObject(enemy);
    AddObject(m_FollowCamera);

    if (m_FollowCamera && m_FollowCamera->GetCameraComponent())
    {
        Vector2 screenPos(static_cast<float>(m_lastDragPos.x), static_cast<float>(m_lastDragPos.y));
        m_FollowCamera->GetCameraComponent()->SetReticleScreenPos(screenPos);
    }
}

void GameScene::Update(float deltatime)
{
    //新規オブジェクトをGameSceneのオブジェクト配列に追加する
    SetSceneObject();

    auto PlayerMove = m_player->GetComponent<MoveComponent>();
    PlayerMove->SetSpeed(setSpeed);
    //----------------- レティクルのドラッグ処理 -----------------
    if (Input::IsMouseLeftPressed())
    {
        m_isDragging = true;
    }

    if (m_isDragging && Input::IsMouseLeftDown()) 
    {
        m_lastDragPos = Input::GetMousePosition();
        SetReticleByCenter(m_lastDragPos);
    }

    if (!Input::IsMouseLeftDown() && m_isDragging)
    {
        m_isDragging = false;
    }

    //カメラにレティクル座標を渡す（最新のものを渡す）
    if (m_FollowCamera && m_FollowCamera->GetCameraComponent())
    {
        m_FollowCamera->GetCameraComponent()->SetReticleScreenPos(Vector2((float)m_lastDragPos.x, (float)m_lastDragPos.y));
    }

    //全オブジェクト Update を一回だけ実行（重要）
    for (auto& obj : m_GameObjects)
    {
        if (obj) obj->Update(deltatime);
    }

    //全オブジェクト Update を一回だけ実行（重要）
    for (auto& obj : m_TextureObjects)
    {
        if (obj) obj->Update(deltatime);
    }

    for (auto& obj : m_GameObjects)
    {
        if (!obj) continue;
        auto collider = obj->GetComponent<ColliderComponent>();
        if (collider)
        {
            CollisionManager::RegisterCollider(collider.get());
        }
    }

    
    Vector3 ppos = m_player->GetPosition();



}

void GameScene::Draw(float deltatime)
{
    //通常のオブジェクト描画
    for (auto& obj : m_GameObjects)
    {

        if (!obj) { continue; }
        if (std::dynamic_pointer_cast<Reticle>(obj)) { continue; }
        obj->Draw(deltatime);
    }

    for (auto& obj : m_TextureObjects)
    {

        if (!obj) { continue; }

        if (std::dynamic_pointer_cast<Reticle>(obj)) continue; // HUD はあとで描く
        obj->Draw(deltatime);
    }

    //HUD(レティクル)を最後に描く(深度やブレンド切り替えは内部で処理)
    if (m_reticle)
    {
        m_reticle->Draw(deltatime);
    }
    
    //デバッグ線
    //CollisionManager::DebugDrawAllColliders(gDebug);

    //(もし m_reticleTex 経由の旧描画があるなら、それも最後に)
    if (m_reticleTex && m_reticleTex->GetSRV())
    {
        Vector2 size(m_reticleW, m_reticleH);
        Renderer::DrawReticle(m_reticleTex->GetSRV(), m_lastDragPos, size);
    }


    if (isCollisionDebugMode)
    {
        if (m_debugRenderer && m_FollowCamera && m_FollowCamera->GetCameraComponent())
        {
            auto camComp = m_FollowCamera->GetCameraComponent();
            Matrix view = camComp->GetView();
            Matrix proj = camComp->GetProj();

            // 各オブジェクトのコライダーを登録
            for (auto& obj : m_GameObjects)
            {
                if (!obj) continue;
                auto col = obj->GetComponent<ColliderComponent>();
                if (!col) continue;

                // 色（当たっているなら赤、そうでなければ緑半透明）
                bool hit = col->IsHitThisFrame();
                Vector4 color = hit ? Vector4(1, 0, 0, 1) : Vector4(0, 1, 0, 0.6f);

                // center / size / rotation を取得
                Vector3 center = col->GetCenter();
                Vector3 size = col->GetSize();          // ** full size を期待 **
                Matrix rot = col->GetRotationMatrix(); // AABB は Identity を返すように実装済み

                // DebugRenderer::AddBox は size が fullSize を期待します（内部で 0.5 をかける）
                m_debugRenderer->AddBox(center, size, rot, color);
            }

            // 描画（内部で Clear しているので AddBox は毎フレーム呼ぶ）
            m_debugRenderer->Draw(m_FollowCamera->GetCameraComponent()->GetView(),
                m_FollowCamera->GetCameraComponent()->GetProj());
        }

    }
    
}


void GameScene::Uninit()
{
    m_AddObjects.clear();
    m_DeleteObjects.clear();

    // 各GameObjectの明示的な終了処理を呼ぶ（任意）
    for (auto& obj : m_GameObjects)
    {
       /* if (obj)
            obj->Uninit();*/ // ← GameObject に Uninit() があるなら呼ぶ
    }
    for (auto& obj : m_TextureObjects)
    {
        //if (obj)
            //obj->Uninit();
    }

    // 配列自体を解放（shared_ptr の参照をすべて消す）
    m_GameObjects.clear();
    m_TextureObjects.clear();

    // 他のリストも念のため
    m_AddObjects.clear();
    m_DeleteObjects.clear();
}

void GameScene::AddObject(std::shared_ptr<GameObject> obj)
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

void GameScene::AddTextureObject(std::shared_ptr<GameObject> obj)
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

//void GameScene::RemoveObject(std::shared_ptr<GameObject> obj) 
//{ 
//    if (!obj) return;
//    // 重複 push を防ぐ
//    auto it = std::find_if(m_DeleteObjects.begin(), m_DeleteObjects.end(), [&](const std::shared_ptr<GameObject>& sp) { return sp.get() == obj.get(); });
//    if (it != m_DeleteObjects.end()) return;
//    m_DeleteObjects.push_back(obj);
//
//} 

void GameScene::RemoveObject(GameObject* obj)
{
    //ポインタがないなら処理終わり
    if (!obj) return;

    //-------------まずm_AddObjectsにいるか確認-------------
    //-------------------いるなら取り消す-----------------
   auto itPending = std::find_if(m_AddObjects.begin(), m_AddObjects.end(),
       [&](const std::shared_ptr<GameObject>& sp) { return sp.get() == obj; });
   if (itPending != m_AddObjects.end())
   {
       m_AddObjects.erase(itPending);
       return;
   }
    //-------------次に m_GameObjectsにいるか確認-----------------
    //-------------いるならm_DeleteObjects に登録-----------------
    auto itInScene = std::find_if(m_GameObjects.begin(), m_GameObjects.end(),
        [&](const std::shared_ptr<GameObject>& sp) { return sp.get() == obj; });
    if (itInScene != m_GameObjects.end())
    {
        // 二重登録防止
        auto already = std::find_if(m_DeleteObjects.begin(), m_DeleteObjects.end(),
            [&](const std::shared_ptr<GameObject>& sp) { return sp.get() == obj; });
        if (already == m_DeleteObjects.end())
        {
            m_DeleteObjects.push_back(*itInScene);
        }
        return;
    }
    //------------------------------------------------------------
}

void GameScene::FinishFrameCleanup()
{ 
    // m_DeleteObjects にあるアイテムを m_GameObjects から削除
    for (auto& delSp : m_DeleteObjects)
    {
        if (!delSp) 
        {
            continue; 
        
        }
        auto it = std::find_if(m_GameObjects.begin(), m_GameObjects.end(),
            [&](const std::shared_ptr<GameObject>& sp) { return sp.get() == delSp.get(); });
        if (it != m_GameObjects.end())
        {
            // オプション: Scene 参照をクリアしたい場合
            // (*it)->SetScene(nullptr);

            m_GameObjects.erase(it);
            //shared_ptr をここで破棄 -> 実際の破棄は参照カウント次第
        }
        // もしオブジェクトがまだ m_AddObjects に入っているケースがあるなら（安全策）
        auto itPending = std::find_if(m_AddObjects.begin(), m_AddObjects.end(),
            [&](const std::shared_ptr<GameObject>& sp) { return sp.get() == delSp.get(); });
        if (itPending != m_AddObjects.end())
        {
            m_AddObjects.erase(itPending);
        }
    }

    m_DeleteObjects.clear();
} 

void GameScene::SetSceneObject()
{ 
    if (!m_AddObjects.empty())
    { 
        //一括追加
        m_GameObjects.reserve(m_GameObjects.size() + m_AddObjects.size()); 
        m_GameObjects.insert(m_GameObjects.end(), std::make_move_iterator(m_AddObjects.begin()), std::make_move_iterator(m_AddObjects.end())); 
        m_AddObjects.clear();
    } 
}
