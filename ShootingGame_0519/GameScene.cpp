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
#include "FloorComponent.h"
#include "PlayAreaComponent.h"

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

    static float speed = 25.0f;

    static Vector3 Rot = { 0,0,0 };
    
    ImGui::SliderFloat("PlayerSpeed", &speed, 0.1f, 75.0f);

    ImGui::SliderFloat("PlayerRotX", &Rot.x, -180.0f, 180.0f);
    ImGui::SliderFloat("PlayerRotY", &Rot.y, -180.0f, 180.0f);
    ImGui::SliderFloat("PlayerRotZ", &Rot.z, -180.0f, 180.0f);

    setSpeed = speed;

    setRot = Rot;

    ImGui::End();
}

void GameScene::Init()
{    
    //デバッグMODE SELECT
    DebugUI::RedistDebugFunction([this]() {DebugCollisionMode();});

    DebugUI::RedistDebugFunction([this]() {DebugSetPlayerSpeed();});

    //DebugRendererの初期化
    m_debugRenderer = std::make_unique<DebugRenderer>();
    m_debugRenderer->Initialize(Renderer::GetDevice(), Renderer::GetDeviceContext(),
        L"DebugLineVS.cso", L"DebugLinePS.cso");

    //--------------------------プレイヤー作成---------------------------------
    m_player = std::make_shared<Player>();
    m_player->SetPosition({ 0.0f, 0.0f, 0.0f });
    m_player->SetRotation({ 80.0,0.0,0.0 });
    m_player->SetScale({ 0.3f, 0.3f, 0.3f });
    m_player->Initialize();
    auto moveComp = m_player->GetComponent<MoveComponent>();

    auto playAreaComp = std::make_shared<PlayAreaComponent>();

    /*playAreaComp->SetMinY(-3.5f);
    playAreaComp->SetMaxY(20.0f);
    moveComp->SetPlayArea(playAreaComp.get());*/

    //-------------------------敵生成--------------------------------
    m_enemySpawner = std::make_unique<EnemySpawner>(this);
    m_enemySpawner->patrolCfg.spawnCount = 1;
    m_enemySpawner->circleCfg.spawnCount = 0;
    m_enemySpawner->turretCfg.spawnCount = 0;

    enemyCount = m_enemySpawner->patrolCfg.spawnCount + m_enemySpawner->circleCfg.spawnCount + m_enemySpawner->turretCfg.spawnCount;

    m_enemySpawner->SetWaypoints({
        {   0.0f, 20.0f,  0.0f },
        {  20.0f, 20.0f,  0.0f },
        {  20.0f, 20.0f,20.0f },
        {   0.0f, 20.0f,20.0f }});
    m_enemySpawner->SetWaypoints({
        {  60.0f, 10.0f,  0.0f },
        {  60.0f, 10.0f,  0.0f },
        {  60.0f, 10.0f,100.0f },
        {   0.0f, 10.0f,200.0f }});
    m_enemySpawner->SetWaypoints({
        {   0.0f, 0.0f,   0.0f },
        {  75.0f, 0.0f,   0.0f },
        {  75.0f, 0.0f, 75.0f },
        {   0.0f, 0.0f, 75.0f }});

    m_enemySpawner->SetCircleCenter({ 50.0f, 0.0f,0.0f});
    m_enemySpawner->SetCircleCenter({ -30.0f,20.0f,0.0f});

    m_enemySpawner->SetRadius(20.0f);
    m_enemySpawner->SetRadius(50.0f);

    m_enemySpawner->SetTurretPos({ 20.0f,20.0f,0.0f });
    m_enemySpawner->SetTurretPos({ 0.0f,40.0f,0.0f });

    m_enemySpawner->turretCfg.target = m_player;

    m_enemySpawner->EnsurePatrolCount();
    m_enemySpawner->EnsureCircleCount();
    m_enemySpawner->EnsureTurretCount();

    //------------------スカイドーム作成-------------------------

    m_SkyDome = std::make_shared<SkyDome>("Asset/SkyDome/SkyDome_03.png");
    m_SkyDome->Initialize();

    //-----------------------床制作------------------------------

    auto floorObj = std::make_shared<GameObject>();
    floorObj->SetPosition(Vector3(0, -5, 0));
    floorObj->SetRotation(Vector3(0, 0, 0));
    floorObj->SetScale(Vector3(75, 75, 75));

    // AddComponent で床コンポーネントを追加（テクスチャパスは任意）
    auto floorComp = floorObj->AddComponent<FloorComponent>();

    floorComp->SetGridTexture("Asset/Texture/grid01.jpeg", 1, 1);

    floorObj->Initialize();

    AddObject(floorObj);

    //--------------------------建物生成-----------------------------

    m_buildingSpawner = std::make_unique<BuildingSpawner>(this);
    BuildingConfig bc;
    bc.modelPath = "Asset/Build/wooden watch tower2.obj";
    bc.count = 1;
    bc.areaWidth = 20.0f;
    bc.areaDepth = 20.0f;
    bc.spacing = 20.0f;
    bc.randomizeRotation = true;
    bc.minScale = 0.9f;
    bc.maxScale = 1.2f;

    m_buildingSpawner->Spawn(bc);

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
    AddObject(m_FollowCamera);

    if (m_FollowCamera && m_FollowCamera->GetCameraComponent())
    {
        Vector2 screenPos(static_cast<float>(m_lastDragPos.x), static_cast<float>(m_lastDragPos.y));
        m_FollowCamera->GetCameraComponent()->SetReticleScreenPos(screenPos);
    }

    // 例: Renderer::Init() の後
    DebugRenderer::Get().Initialize(Renderer::GetDevice(), Renderer::GetDeviceContext());

}

void GameScene::Update(float deltatime)
{
    //新規オブジェクトをGameSceneのオブジェクト配列に追加する
    SetSceneObject();

    if (m_player)
    {
        //m_player->SetRotation(setRot);
    }

    auto PlayerMove = m_player->GetComponent<MoveComponent>();
    if (PlayerMove)
    {
        PlayerMove->SetSpeed(setSpeed);
    }

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

    if (enemyCount <= 0)
    {
        SceneManager::SetChangeScene("TitleScene");
    }

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

            //各オブジェクトのコライダーを登録
            for (auto& obj : m_GameObjects)
            {
                if (!obj) continue;
                auto col = obj->GetComponent<ColliderComponent>();
                if (!col) continue;

                //色（当たっているなら赤、そうでなければ緑半透明）
                bool hit = col->IsHitThisFrame();
                Vector4 color = hit ? Vector4(1, 0, 0, 1) : Vector4(0, 1, 0, 0.6f);

                Vector3 center = col->GetCenter();
                Vector3 size = col->GetSize();         
                Matrix rot = col->GetRotationMatrix(); 

                //DebugRenderer::AddBox は size が fullSize を期待します
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
    if (m_enemySpawner)
    {
        m_enemySpawner.reset();
    }

    //DebugRenderer をクリアして破棄
    if (m_debugRenderer)
    {
        m_debugRenderer->Clear();

        m_debugRenderer.reset();
    }

    //GameObject解放
    for (auto& obj : m_GameObjects)
    {
        if (!obj) { continue; }
        // GameObject::Uninit を実装しておくこと（下で例示）
        obj->Uninit();
        obj->SetScene(nullptr);
    }

    //テクスチャオブジェクト解放
    for (auto& obj : m_TextureObjects)
    {
        if (!obj) { continue; }
        obj->Uninit();
        obj->SetScene(nullptr);
    }

    //vectors をクリアして shared_ptr の参照カウントを下げる
    m_GameObjects.clear();
    m_TextureObjects.clear();
    m_AddObjects.clear();
    m_DeleteObjects.clear();

    //個別メンバ（player, camera, etc.）を reset
    m_player.reset();
    m_FollowCamera.reset();
    m_SkyDome.reset();
    m_reticleObj.reset();
    m_reticleTex.reset();
    m_reticle.reset();
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
    //m_DeleteObjectsにあるアイテムを削除
    for (auto& delSp : m_DeleteObjects)
    {
        if (!delSp){ continue; }

        //m_GameObjects から探す
        auto it = std::find_if(m_GameObjects.begin(), m_GameObjects.end(),
            [&](const std::shared_ptr<GameObject>& sp) { return sp.get() == delSp.get(); });

        if (it != m_GameObjects.end())
        {
            if (auto enemy = std::dynamic_pointer_cast<Enemy>(*it))
            {
                enemyCount -= 1;
            }

            //Uninit
            (*it)->Uninit();

            //シーン参照を切る
            (*it)->SetScene(nullptr);

            m_GameObjects.erase(it);
        }

        //m_AddObjectsにまだあるなら削除
        auto itPending = std::find_if(m_AddObjects.begin(), m_AddObjects.end(),
            [&](const std::shared_ptr<GameObject>& sp) { return sp.get() == delSp.get(); });
        if (itPending != m_AddObjects.end())
        {
            // 追加前に削除予定だったオブジェクトなら Uninit して erase
            (*itPending)->Uninit();
            (*itPending)->SetScene(nullptr);
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
