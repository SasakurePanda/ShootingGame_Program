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
#include "HitPointCompornent.h"
#include "DebugPlayer.h"

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

    static float speed = 35.0f;

    static Vector3 Rot = { 0,0,0 };
    
    ImGui::SliderFloat("PlayerSpeed", &speed, 0.1f, 75.0f);

    ImGui::SliderFloat("PlayerRotX", &Rot.x, -180.0f, 180.0f);
    ImGui::SliderFloat("PlayerRotY", &Rot.y, -180.0f, 180.0f);
    ImGui::SliderFloat("PlayerRotZ", &Rot.z, -180.0f, 180.0f);

    setSpeed = speed;

    setRot = Rot;

    ImGui::End();
}

void GameScene::DebugSetAimDistance()
{
    ImGui::Begin("DebugAimDistance");

    static float aimDistance = 2000.0f;

    ImGui::SliderFloat("AimDistance", &aimDistance, 2000.0f, 7500.0f);

    setAimDistance = aimDistance;

    ImGui::End();
}

static bool RaySphereIntersect(const DirectX::SimpleMath::Vector3& rayOrigin,
    const DirectX::SimpleMath::Vector3& rayDir,
    const DirectX::SimpleMath::Vector3& sphereCenter,
    float sphereRadius,
    float& outT)
{
    using namespace DirectX::SimpleMath;
    Vector3 m = rayOrigin - sphereCenter;
    float b = m.Dot(rayDir);
    float c = m.LengthSquared() - sphereRadius * sphereRadius;

    if (c > 0.0f && b > 0.0f) { return false; }

    float discr = b * b - c;
    if (discr < 0.0f) { return false; }

    float t = -b - sqrtf(discr);
    if (t < 0.0f)
    {
        t = -b + sqrtf(discr);
    }
    if (t < 0.0f) { return false; }

    outT = t;
    return true;
}

bool GameScene::Raycast(const DirectX::SimpleMath::Vector3& origin,
    const DirectX::SimpleMath::Vector3& dir,
    float maxDistance,
    RaycastHit& outHit,
    std::function<bool(GameObject*)> predicate,
    GameObject* ignore)
{
    using namespace DirectX::SimpleMath;

    // Normalize dir for distance correctness
    Vector3 ndir = dir;
    if (ndir.LengthSquared() <= 1e-6f)
    {
        return false;
    }
    ndir.Normalize();

    float bestT = std::numeric_limits<float>::infinity();
    std::shared_ptr<GameObject> bestObj;

    for (auto& obj : m_GameObjects)
    {
        if (!obj) { continue; }
        if (obj.get() == ignore) { continue; }

        if (predicate)
        {
            if (!predicate(obj.get()))
            {
                continue;
            }
        }

        // Candidate check: prefer using collider components (AABB/sphere) if present
        // Example: if GameObject has a method GetBoundingRadius use sphere test,
        // otherwise if AABBColliderComponent exists use AABB raycast, etc.
        // Here we show a fallback using sphere: if object provides GetBoundingRadius().
        float t;
        float radius = 0.0f;

        // try dynamic_cast to a component that gives us a bounding radius (example)
        // if your GameObject has GetBoundingRadius method, call it instead.
        bool hasSphere = false;
        // Example pseudo-check: if (obj->HasComponent<SphereCollider>()) ...
        // For now, try to call a method if exists (adjust to your codebase)
        // ----
        // Fallback: if object exposes a method GetBoundingRadius() - adapt as needed:
        // float r = obj->GetBoundingRadius(); // uncomment if exists
        // hasSphere = (r > 0.0f);
        // radius = r;

        // If you have Enemy::GetBoundingRadius we can still use it:
        {
            Enemy* e = dynamic_cast<Enemy*>(obj.get());
            if (e)
            {
                radius = e->GetBoundingRadius();
                hasSphere = true;
            }
        }

        if (hasSphere)
        {
            if (RaySphereIntersect(origin, ndir, obj->GetPosition(), radius, t))
            {
                if (t >= 0.0f && t <= maxDistance && t < bestT)
                {
                    bestT = t;
                    bestObj = obj;
                }
            }
        }
        else
        {
            // Optionally, test AABB if your GameObject has AABB component
            // AABBColliderComponent* aabb = obj->GetComponent<AABBColliderComponent>();
            // if (aabb) { if (RayAABBIntersect(...)) { ... } }
            // For brevity, skip if no collider info
        }
    }

    if (bestObj)
    {
        outHit.hitObject = bestObj;
        outHit.distance = bestT;
        outHit.position = origin + ndir * bestT;
        outHit.normal = (outHit.position - bestObj->GetPosition());
        if (outHit.normal.LengthSquared() > 1e-6f)
        {
            outHit.normal.Normalize();
        }
        return true;
    }

    return false;
}

bool GameScene::RaycastForAI(const DirectX::SimpleMath::Vector3& origin,
    const DirectX::SimpleMath::Vector3& dir,
    float maxDistance,
    RaycastHit& outHit,
    GameObject* ignore)
{
    using namespace DirectX::SimpleMath;

    Vector3 ndir = dir;
    if (ndir.LengthSquared() <= 1e-6f)
        return false;
    ndir.Normalize();

    float bestT = std::numeric_limits<float>::infinity();
    std::shared_ptr<GameObject> bestObj;

    for (auto& obj : m_GameObjects)
    {
        if (!obj) continue;
        if (obj.get() == ignore) continue;

        // ★ コライダーを持っていないなら無視
        auto obb = obj->GetComponent<OBBColliderComponent>();
        auto aabb = obj->GetComponent<AABBColliderComponent>();
        bool hasCollider = (obb || aabb);

        // Enemy の簡易球 or Building の AABB など…
        float radius = 0.0f;
        bool hasSphere = false;

        if (auto e = dynamic_cast<Enemy*>(obj.get()))
        {
            radius = e->GetBoundingRadius();
            if (radius > 0.0f) hasSphere = true;
        }
        else if (hasCollider)
        {
            // ★ コライダーからざっくり半径を作る（対角長の半分）
            //    きっちりした Ray vs OBB/AABB は後で実装でもOK
            Vector3 center = obj->GetPosition();
            Vector3 extents;

            if (obb)
            {
                extents = obb->GetSize() * 0.5f; // size が全長なら半分で半径相当
            }
            else // AABB
            {
                Vector3 size = aabb->GetSize();
                extents = size * 0.5f;
            }

            radius = extents.Length(); // 対角線の長さ ≒ おおざっぱな半径
            hasSphere = (radius > 0.0f);
        }

        if (!hasSphere) continue;

        float t;
        if (RaySphereIntersect(origin, ndir, obj->GetPosition(), radius, t))
        {
            if (t >= 0.0f && t <= maxDistance && t < bestT)
            {
                bestT = t;
                bestObj = obj;
            }
        }
    }

    if (bestObj)
    {
        outHit.hitObject = bestObj;
        outHit.distance = bestT;
        outHit.position = origin + ndir * bestT;

        // 簡易法線（中心からの方向）
        outHit.normal = outHit.position - bestObj->GetPosition();
        if (outHit.normal.LengthSquared() > 1e-6f)
            outHit.normal.Normalize();

        return true;
    }

    return false;
}

void GameScene::Init()
{    
    //デバッグMODE SELECT
    DebugUI::RedistDebugFunction([this]() {DebugCollisionMode();});

    DebugUI::RedistDebugFunction([this]() {DebugSetPlayerSpeed();});
    
    DebugUI::RedistDebugFunction([this]() {DebugSetAimDistance();});

    //DebugRendererの初期化
    m_debugRenderer = std::make_unique<DebugRenderer>();
    m_debugRenderer->Initialize(Renderer::GetDevice(), Renderer::GetDeviceContext(),
        L"DebugLineVS.cso", L"DebugLinePS.cso");
    //--------------------------プレイヤー作成---------------------------------
    m_playArea = std::make_shared<PlayAreaComponent>();
    m_playArea->SetScene(this); // PlayArea がシーンを利用する場合
    m_playArea->SetBounds({ -150.0f, -1.0f, -150.0f }, { 150.0f, 150.0f, 150.0f });
    m_playArea->SetGroundY(-7.0f);
    
    m_FollowCamera = std::make_shared<CameraObject>();
    m_FollowCamera->Initialize();

    m_player = std::make_shared<Player>();
    m_player->SetPosition({ 0.0f, 0.0f, 0.0f });
    m_player->SetRotation({ 80.0,0.0,0.0 });
    m_player->SetScale({ 0.3f, 0.3f, 0.3f });
    m_player->Initialize();

    auto moveComp = m_player->GetComponent<MoveComponent>();

    if (moveComp)
    {
        // PlayArea を渡す（PlayArea は shared_ptr でシーンが保持している）
        moveComp->SetPlayArea(m_playArea.get());

        // obstacleTester を PlayArea の RaycastObstacle に接続
        //moveComp->SetObstacleTester([this](const DirectX::SimpleMath::Vector3& start,
        //    const DirectX::SimpleMath::Vector3& dir,
        //    float len,
        //    DirectX::SimpleMath::Vector3& outNormal,
        //    float& outDist) -> bool
        //    {
        //        if (!m_playArea) { return false; }
        //        return m_playArea->RaycastObstacle(start, dir, len, outNormal, outDist, /*ignore*/ nullptr);
        //    });
    }

    //-------------------------敵生成--------------------------------
    m_enemySpawner = std::make_unique<EnemySpawner>(this);
    m_enemySpawner->patrolCfg.spawnCount = 2;

    enemyCount = m_enemySpawner->patrolCfg.spawnCount + m_enemySpawner->circleCfg.spawnCount + m_enemySpawner->turretCfg.spawnCount;

    m_enemySpawner->SetWaypoints({
        {   0.0f, 20.0f,  0.0f },
        {   0.0f, 20.0f,  0.0f },
        {   0.0f, 20.0f,  0.0f },
        {   0.0f, 20.0f,  0.0f }});
    m_enemySpawner->SetWaypoints({
        {   0.0f, 20.0f,  0.0f },
        { -60.0f, 20.0f,  0.0f },
        { -60.0f, 20.0f,-60.0f },
        {   0.0f, 20.0f,-60.0f }});
    m_enemySpawner->SetWaypoints({
        {   0.0f,  0.0f,   0.0f },
        { 120.0f,  0.0f,   0.0f },
        { 120.0f,  0.0f, 120.0f },
        {   0.0f,  0.0f, 120.0f }});

    m_enemySpawner->EnsurePatrolCount();

    m_enemySpawner->fleeCfg.spawnCount = 1;
    m_enemySpawner->fleeCfg.maxSpeed = 48.0f;
    m_enemySpawner->fleeCfg.maxForce = 50.0f;
    m_enemySpawner->fleeCfg.fleeStrength = 1.0f;
    m_enemySpawner->fleeCfg.lookahead = 20.0f;
    m_enemySpawner->fleeCfg.feelerCount = 9;
    m_enemySpawner->fleeCfg.feelerSpread = DirectX::XM_PI / 4.0f;
    m_enemySpawner->fleeCfg.player = m_player;

    //m_enemySpawner->EnsureFleeCount();
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
    bc.count = 6;
    bc.areaWidth = 300.0f;
    bc.areaDepth = 300.0f;
    bc.spacing = 30.0f;          //建物間に20単位の余裕を入れる
    bc.randomizeRotation = true;
    bc.minScale = 5.0f;
    bc.maxScale = 10.0f;
    bc.footprintSizeX = 6.0f;    //必要ならモデルに合わせて調整
    bc.footprintSizeZ = 6.0f;
    bc.baseColliderSize = { 3.0f, 17.0f, 3.0f };
    bc.maxAttemptsPerBuilding = 50;

    int placed = m_buildingSpawner->Spawn(bc);

    /*bc.modelPath = "Asset/Build/Rock1.obj";
    m_buildingSpawner->Spawn(bc);*/

    //-------------------------レティクル作成-------------------------------------
    m_reticle = std::make_shared<Reticle>(L"Asset/UI/26692699.png", m_reticleW);
    RECT rc{};
    GetClientRect(Application::GetWindow(), &rc);
    float x = (rc.right - rc.left) / 2;
    float y = (rc.bottom - rc.top) / 2;
    m_reticle->Initialize();
    
	//--------------------------HPバー作成-------------------------------------
	auto hpUI = std::make_shared<HPBar>(L"Asset/UI/HPBar01.png", L"Asset/UI/HPGauge01.png", 100.0f, 475.0f);
	hpUI->SetScreenPos(30.0f, 200.0f);
    hpUI->Initialize();

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
        cameraComp->SetPlayArea(m_playArea.get());
        cameraComp->SetDistance(5.0f);
    }


    // 弱参照を作る（ラムダ内で lock して使う）
    std::weak_ptr<HPBar> wHpUI = hpUI;

    // cameraComp が生ポインタ（raw）ならそのままキャプチャして良い。
    // cameraComp が shared_ptr なら同様に weak_ptr にしておくのが安全。
    auto hp = m_player->GetComponent<HitPointComponent>();
    if (hp)
    {
        std::weak_ptr<HitPointComponent> wPlayerHP = hp;
        std::weak_ptr<HPBar> wHpUI = hpUI;


        hp->SetOnDamaged([wHpUI, wPlayerHP, cameraComp](const DamageInfo& info)
            {
                // カメラシェイク（cameraComp が raw pointer なら null チェック）
                if (cameraComp)
                {
                    cameraComp->Shake(7.5f, 0.5f , FollowCameraComponent::ShakeMode::Horizontal);
                }

                 //HPUI 更新：まず weak -> shared にする
                if (auto bar = wHpUI.lock())
                {
                    if (auto playerHP = wPlayerHP.lock())
                    {
                        // HitPointComponent 側に GetHP/GetMaxHP があれば使う
                        float cur = playerHP->GetHP();
                        float max = playerHP->GetMaxHP();
                        bar->SetHP(cur, max);
                    }
                    else
                    {
                        // 万が一 playerHP が無ければ DamageInfo に current/max が入っていれば使う
                    }
                }
            });
    }

    cameraComp->SetDistance(15.0f);

    m_GameObjects.insert(m_GameObjects.begin(), m_SkyDome);

    m_FollowCamera->GetCameraComponent()->SetTarget(m_player.get());

    //AddTextureObject(HPbar);
    AddTextureObject(m_reticle);
    AddTextureObject(hpUI);

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

    //static float currentBlur = 0.0f;

    // MoveComponent をキャッシュ
    if (!m_playerMove)
    {
        m_playerMove = m_player->GetComponent<MoveComponent>();
    }

    if (m_playerMove)
    {
        //--- 今ブースト中か？
        bool boosting = m_playerMove->GetBoostingState();

        //--- ブラーの目標値
        float targetBlur = boosting ? 1.0f : 0.0f; // ガッツリなら 1.0f

        //--- 補間速度
        float interpSpeed = 8.0f;

        //--- currentBlur を目標値へ補間（指数補間）
        //currentBlur += (targetBlur - currentBlur) * std::min(1.0f, interpSpeed * deltatime);

        //---------------------------------------------------------
        // ★ Renderer のポストプロセス設定に反映する
        //---------------------------------------------------------
        PostProcessSettings pp = Renderer::GetPostProcessSettings();

        // ブラー強度
        //pp.motionBlurAmount = currentBlur;        // 0〜1

        // 画面上の伸びる方向（仮例：前方向）
        pp.motionBlurDir = { 0.0f, -1.0f };       // 奥方向に伸ばしたい場合

        // ブラーの長さ（大きいほど “ドバーッ” と伸びる）
        pp.motionBlurStretch = 0.03f;             // （調整ポイント）

        Renderer::SetPostProcessSettings(pp);
    }

    //フレーム先頭で前フレームの登録を消す
    CollisionManager::Clear();

    //新規オブジェクトをGameSceneのオブジェクト配列に追加する
    SetSceneObject();

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

    //----------------------------------------------

    if (m_FollowCamera && m_reticle)
    {
        auto followCan = m_FollowCamera->GetComponent<FollowCameraComponent>();
        followCan->SetReticleScreenPos(m_reticle->GetScreenPos());
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

    //当たり判定チェック実行
    CollisionManager::CheckCollisions();

    //-------------------------------------------------
    // ★ 衝突で溜めた MTV を 1 回だけ適用する
    //-------------------------------------------------
    if (!m_playerMove && m_player)
    {
        m_playerMove = m_player->GetComponent<MoveComponent>();
    }
    if (m_playerMove)
    {
        m_playerMove->ApplyCollisionPush();
    }

}

void GameScene::Draw(float dt)
{
    DrawWorld(dt);
    DrawUI(dt);
}

void GameScene::DrawWorld(float deltatime)
{
    for (auto& obj : m_GameObjects)
    {
        if (!obj) { continue; }
        if (std::dynamic_pointer_cast<Reticle>(obj)) { continue; }
        obj->Draw(deltatime);
    }

    // コリジョンデバッグ描画（ワールド空間なのでここに入れる）
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

                bool hit = col->IsHitThisFrame();
                Vector4 color = hit ? Vector4(1, 0, 0, 1) : Vector4(0, 1, 0, 0.6f);

                Vector3 center = col->GetCenter();
                Vector3 size = col->GetSize();
                Matrix  rot = col->GetRotationMatrix();

                m_debugRenderer->AddBox(center, size, rot, color);
            }

            // デバッグボックス描画
            m_debugRenderer->Draw(
                m_FollowCamera->GetCameraComponent()->GetView(),
                m_FollowCamera->GetCameraComponent()->GetProj()
            );
        }
    }
}

void GameScene::DrawUI(float deltatime)
{
    for (auto& obj : m_TextureObjects)
    {
        if (!obj) { continue; }
        if (std::dynamic_pointer_cast<Reticle>(obj)) continue; // HUD はあとで描く
        obj->Draw(deltatime);
    }

    // HUD(レティクル)を最後に描く
    if (m_reticle)
    {
        m_reticle->Draw(deltatime);
    }

    // 旧レティクル描画が残っている場合
    if (m_reticleTex && m_reticleTex->GetSRV())
    {
        Vector2 size(m_reticleW, m_reticleH);
        Renderer::DrawReticle(m_reticleTex->GetSRV(), m_lastDragPos, size);
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
    if (!obj) { return; }

    //------------------------------
	// コライダー登録解除
    //------------------------------
    if (auto col = obj->GetComponent<ColliderComponent>())
    {
        CollisionManager::UnregisterCollider(col.get());
    }

    //------------------------------
    // m_AddObjectsにいるか確認
    // いるなら取り消す
    //------------------------------
   auto itPending = std::find_if(m_AddObjects.begin(), m_AddObjects.end(),
       [&](const std::shared_ptr<GameObject>& sp) { return sp.get() == obj; });
   if (itPending != m_AddObjects.end())
   {
       m_AddObjects.erase(itPending);
       return;
   }
    //---------------------------------
    // m_GameObjectsにいるか確認
    // いるならm_DeleteObjects に登録
    // ---------------------------------
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
