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
#include "PushOutComponent.h"
#include "SphereColliderComponent.h"
#include "EffectManager.h"

void GameScene::DebugCollisionMode()
{
    static int selected = 1;

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

        float t;
        float radius = 0.0f;

        bool hasSphere = false;

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

        //コライダーを持っていないなら無視
        auto obb = obj->GetComponent<OBBColliderComponent>();
        auto aabb = obj->GetComponent<AABBColliderComponent>();
        bool hasCollider = (obb || aabb);

        //Enemy の簡易球 or Building の AABB など…
        float radius = 0.0f;
        bool hasSphere = false;

        if (auto e = dynamic_cast<Enemy*>(obj.get()))
        {
            radius = e->GetBoundingRadius();
            if (radius > 0.0f) hasSphere = true;
        }
        else if (hasCollider)
        {
            //コライダーからざっくり半径を作る（対角長の半分）
            //きっちりした Ray vs OBB/AABB は後で実装でもOK
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
    m_playArea->SetBounds({ -300.0f, -1.0f, -300.0f }, { 300.0f, 200.0f, 300.0f });
    m_playArea->SetGroundY(-7.0f);
    
    m_FollowCamera = std::make_shared<CameraObject>();

    auto followCamComp = m_FollowCamera->AddCameraComponent<FollowCameraComponent>();

    AddObject(m_FollowCamera);

    m_FollowCamera->Initialize();

    m_player = std::make_shared<Player>();
    m_player->Initialize();

    auto moveComp = m_player->GetComponent<MoveComponent>();

    if (moveComp)
    {
        moveComp->SetPlayArea(m_playArea.get());
    }

    //-------------------------敵生成--------------------------------
    m_enemySpawner = std::make_unique<EnemySpawner>(this);
    m_enemySpawner->patrolCfg.spawnCount = 1;
	m_enemySpawner->circleCfg.spawnCount = 0;
    m_enemySpawner->turretCfg.spawnCount = 1;
    m_enemySpawner->fleeCfg.spawnCount = 0;

    enemyCount = m_enemySpawner->patrolCfg.spawnCount + m_enemySpawner->circleCfg.spawnCount + m_enemySpawner->turretCfg.spawnCount;

    m_enemySpawner->SetWaypoints(
        { { 80.0f, 20.0f,  0.0f }, 
          { 40.0f, 20.0f,-80.0f }, 
          {-40.0f, 20.0f,-80.0f }, 
          {-80.0f, 20.0f,  0.0f }, 
          { 40.0f, 20.0f, 80.0f },});

    m_enemySpawner->SetWaypoints(
        { { 80.0f, 40.0f,  0.0f },
          { 40.0f, 40.0f,-80.0f },
          {-40.0f, 40.0f,-80.0f },
          {-80.0f, 40.0f,  0.0f },
          { 40.0f, 40.0f, 80.0f }, });

    m_enemySpawner->SetWaypoints(
        { { 125.0f, 90.0f,    0.0f },
          {  62.5f, 90.0f, -125.0f },
          { -62.5f, 90.0f,  125.0f },
          { -62.5f, 90.0f, -125.0f },
          {  62.5f, 90.0f,  125.0f },
          {-125.0f, 90.0f,  -62.5f },
          {-125.0f, 90.0f,   62.5f }});

    m_enemySpawner->SetWaypoints(
        { { 62.0f, 120.0f,    0.0f },
          {  62.5f, 120.0f, -125.0f },
          { 125.5f, 120.0f,  125.0f },
          { -62.5f, 120.0f, -125.0f },
          {-125.5f, 120.0f,  125.0f },
          {  62.0f, 120.0f,  -62.5f },
          {-125.0f, 120.0f,   62.5f } });

    m_enemySpawner->EnsurePatrolCount();
	m_enemySpawner->turretCfg.target = m_player;
	m_enemySpawner->turretCfg.bulletSpeed = 80.0f;
    m_enemySpawner->SetTurretPos({ 100.0f,100.0f,0.0f });
    m_enemySpawner->SetTurretPos({ -100.0f,100.0f,0.0f });

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

    bc.fixedPositions =
    {
        { 125.0f, -12.0f,    0.0f },
        {  62.5f, -12.0f, -125.0f },
        { -62.5f, -12.0f,  125.0f },
        { -62.5f, -12.0f, -125.0f },
        {  62.5f, -12.0f,  125.0f },
        {-125.0f, -12.0f,  -62.5f },
        {-125.0f, -12.0f,   62.5f },
        {   0.0f, -12.0f,    0.0f },
    };

    int placed = m_buildingSpawner->Spawn(bc);

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

    //-------------ミニマップ設定----------------
	m_miniMapBgSRV       = TextureManager::Load("Asset/UI/minimap_Background.png");
    m_miniMapPlayerSRV   = TextureManager::Load("Asset/UI/mimimap_player.png");
    m_miniMapEnemySRV    = TextureManager::Load("Asset/UI/mimimap_enemy.png");
    m_miniMapBuildingSRV = TextureManager::Load("Asset/UI/mimimap_building.png");

    m_miniMapUi = std::make_shared<GameObject>();
    m_miniMap = m_miniMapUi->AddComponent<MiniMapComponent>().get();

    m_miniMap->SetScreenPosition(1008.0f, 16.0f);
    m_miniMap->SetSize(256.0f, 256.0f);
    m_miniMap->SetCoverageRadius(200.0f);
    m_miniMap->SetRotateWithPlayer(true);
    m_miniMap->SetIconSize(10.0f);

    m_miniMap->SetBackgroundSRV(m_miniMapBgSRV);
	m_miniMap->SetPlayerIconSRV(m_miniMapPlayerSRV);
    m_miniMap->SetEnemyIconSRV(m_miniMapEnemySRV);
    m_miniMap->SetBuildingIconSRV(m_miniMapBuildingSRV);

    m_miniMap->SetPlayer(m_player.get()); // m_playerがshared_ptr<GameObject>想定

    //---------------------------------------------
	
    m_GameObjects.insert(m_GameObjects.begin(), m_SkyDome);

    m_FollowCamera->GetFollowCameraComponent()->SetTarget(m_player.get());

    //AddTextureObject(HPbar);
    AddTextureObject(m_reticle);
    AddTextureObject(hpUI);
    AddTextureObject(m_miniMapUi);

    AddObject(m_player);
    AddObject(m_FollowCamera);

    if (m_FollowCamera && m_FollowCamera->GetFollowCameraComponent())
    {
        Vector2 screenPos(static_cast<float>(m_lastDragPos.x), static_cast<float>(m_lastDragPos.y));
        m_FollowCamera->GetFollowCameraComponent()->SetReticleScreenPos(screenPos);
    }

    // 例: Renderer::Init() の後
    DebugRenderer::Get().Initialize(Renderer::GetDevice(), Renderer::GetDeviceContext());

}

void GameScene::Update(float deltatime)
{
    static float currentBlur = 0.0f;

    // MoveComponent をキャッシュ
    if (!m_playerMove)
    {
        m_playerMove = m_player->GetComponent<MoveComponent>();
    }

    if (m_playerMove)
    {
        //ブーストの“強さ”を 0～1 で取得（MoveComponent 側で用意済み）
        float boostIntensity = m_playerMove->GetBoostIntensity(); // 0～1

        // ② ブラーの目標値（強すぎるなら 0.7f とかにしてもOK）
        float targetBlur = boostIntensity * 1.0f;

        // ③ 補間（急にON/OFFせず、なめらかに変化）
        float interpSpeed = 6.0f; // 大きいほど追従が速い
        float alpha = std::min(1.0f, interpSpeed * deltatime);
        currentBlur += (targetBlur - currentBlur) * alpha;

        //---------------------------------------------------------
        // ★ Renderer のポストプロセス設定に反映する
        //---------------------------------------------------------
        PostProcessSettings pp = Renderer::GetPostProcessSettings();

        // ブラー強度（0～1）
        pp.motionBlurAmount = currentBlur;

        // 画面上の伸びる方向（とりあえず前方へ）
        pp.motionBlurDir = { 0.0f, -1.0f };

        // ブラーの伸びる長さ（調整ポイント）
        pp.motionBlurStretch = 0.03f;

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

    /*if (m_isDragging && Input::IsMouseLeftDown()) 
    {
        m_lastDragPos = Input::GetMousePosition();
        SetReticleByCenter(m_lastDragPos);
    }*/

    if (!Input::IsMouseLeftDown() && m_isDragging)
    {
        m_isDragging = false;
    }

    //カメラにレティクル座標を渡す（最新のものを渡す）
    if (m_FollowCamera && m_FollowCamera->GetCameraComponent())
    {
        m_FollowCamera->GetFollowCameraComponent()->SetReticleScreenPos(Vector2((float)m_lastDragPos.x, (float)m_lastDragPos.y));
    }

    //----------------------------------------------
    
    auto followCan = m_FollowCamera->GetComponent<FollowCameraComponent>();
    if (m_FollowCamera && m_reticle)
    {
        followCan->SetReticleScreenPos(m_reticle->GetScreenPos());
    }

    followCan->SetReticleScreen(m_reticle->camera);

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

    for (auto& obj : m_GameObjects)
    {
        if (!obj) { continue; }

        auto push = obj->GetComponent<PushOutComponent>();
        if (push)
        {
            push->ApplyPush();
        }
    }

    if (m_miniMap)
    {
        std::vector<GameObject*> enemies;
        std::vector<GameObject*> buildings;

        for (std::shared_ptr<GameObject> obj : m_GameObjects)
        {
            if (!obj)
            {
                continue;
            }

            if (auto enemy = std::dynamic_pointer_cast<Enemy>(obj))
            { 
                enemies.push_back(obj.get());
            }
            else if (auto building = std::dynamic_pointer_cast<Building>(obj))
            { 
                buildings.push_back(obj.get());
            }
        }

        m_miniMap->SetEnemies(enemies);
        m_miniMap->SetBuildings(buildings);
    }


	auto hp = m_player->GetComponent<HitPointComponent>();
	if (enemyCount <= 0)
    {
        SceneManager::SetCurrentScene("ResultScene");
    }
	
    if (hp->GetHP() <= 0)
    {
        SceneManager::SetCurrentScene("ResultScene02");
    }

    if (Input::IsKeyDown('P'))
    {
        //EffectManager::SpawnExplosion(m_player->GetPosition());
    }

}

void GameScene::Draw(float dt)
{
    DrawWorld(dt);
    EffectManager::Draw3D(dt);
    DrawUI(dt);
}

void GameScene::DrawWorld(float deltatime)
{
    if (m_FollowCamera && m_FollowCamera->GetCameraComponent())
    {
        auto cam = m_FollowCamera->GetCameraComponent();
        Renderer::SetViewMatrix(cam->GetView());
        Renderer::SetProjectionMatrix(cam->GetProj());
    }

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
                if (!obj) { continue; }
                auto col = obj->GetComponent<ColliderComponent>();
                if (!col) { continue; }

                bool hit = col->IsHitThisFrame();
                Vector4 color = hit ? Vector4(1, 0, 0, 1) : Vector4(0, 1, 0, 0.6f);

                Vector3 center = col->GetCenter();

                if (col->GetColliderType() == ColliderType::AABB)
                {
                    auto aabb = static_cast<AABBColliderComponent*>(col.get());
                    Vector3 mn = aabb->GetMin();
                    Vector3 mx = aabb->GetMax();
                    Vector3 size = (mx - mn);              // フルサイズ
                    m_debugRenderer->AddBox(center, size, Matrix::Identity, color);
                }
                else if (col->GetColliderType() == ColliderType::OBB)
                {
                    auto obb = static_cast<OBBColliderComponent*>(col.get());
                    Vector3 size = obb->GetSize();         // フルサイズ
                    Matrix rot = obb->GetRotationMatrix();
                    m_debugRenderer->AddBox(center, size, rot, color);
                }
                else if (col->GetColliderType() == ColliderType::SPHERE)
                {
                    auto sphere = static_cast<SphereColliderComponent*>(col.get());
                    float radius = sphere->GetRadius();
                    m_debugRenderer->AddSphere(center, radius, color, 24);
                }
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
        if (std::dynamic_pointer_cast<Reticle>(obj)) { continue; } // HUD はあとで描く
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
