#include <iostream>
#include "GameScene.h"
#include "Input.h"
#include "DebugGlobals.h" 
#include "renderer.h"
#include "Application.h"
#include "Collision.h"
#include "CollisionManager.h"
#include "SkyDome.h"

void GameScene::Init()
{
    //--------------------------プレイヤー作成---------------------------------
    m_player = std::make_shared<Player>();
    m_player->SetPosition({ 0.0f, 0.0f, 0.0f });
    m_player->SetRotation({ 0.0f,0.0f,0.0f });
    m_player->SetScale({ 0.3f, 0.3f, 0.3f });
    m_player->Initialize();
    auto moveComp = m_player->GetComponent<MoveComponent>();
    //------------------------------------------------------------------------

    //--------------------------エネミー作成---------------------------------
    m_enemy = std::make_shared<Enemy>();
    m_enemy->SetPosition({ 10.0f, 0.0f, 0.0f });
    m_enemy->SetRotation({ 0.0f,0.0f,0.0f });
    m_enemy->SetScale({ 1.0f, 1.0f, 1.0f });
    m_enemy->Initialize();
    //----------------------------------------------------------------------

    // ShootingComponent に this（現在のシーン）を渡す
    auto shootComp = m_player->GetComponent<ShootingComponent>();
    if (shootComp)
    {
        shootComp->SetScene(this);
    }

    //CameraObjectst作成
    m_FollowCamera = std::make_shared<CameraObject>();
    m_FollowCamera->Initialize();
    auto cameraComp = m_FollowCamera->GetComponent<FollowCameraComponent>();

    if (moveComp && cameraComp)
    {
        moveComp->SetCameraView(cameraComp.get()); // ←重要！
    }

    if (shootComp && cameraComp)
    {
        shootComp->SetScene(this);
        shootComp->SetCamera(cameraComp.get()); // ←これが重要！
    }

    auto sky = std::make_shared<SkyDome>("Asset/SkyDome/SkyDome_01.jpg");
    sky->Initialize();
    auto camComp = m_FollowCamera->GetComponent<FollowCameraComponent>();
    if (camComp)
    {
        sky->SetCamera(camComp.get()); // ICameraViewProvider* を受け取る場合
    }
    m_GameObjects.insert(m_GameObjects.begin(), sky);

    m_FollowCamera->GetCameraComponent()->SetTarget(m_player.get());

    m_GameObjects.push_back(m_player);
    m_GameObjects.push_back(m_enemy);
    m_GameObjects.push_back(m_FollowCamera);

    // ---------------- レティクル初期化（Reticle オブジェクトで管理） ----------------
    m_reticle = std::make_shared<Reticle>(L"Asset/UI/26692699.png", m_reticleW);
    m_reticle->Initialize();

    // HUD なのでオブジェクト配列の末尾に入れておく（描画順：後ろにあるものほど最後に描画）
    m_GameObjects.push_back(m_reticle);
    // -------------------------------------------------------------------------------

    if (m_FollowCamera && m_FollowCamera->GetCameraComponent())
    {
        // SimpleMath::Vector2 へ変換して渡す
        Vector2 screenPos(static_cast<float>(m_lastDragPos.x), static_cast<float>(m_lastDragPos.y));
        m_FollowCamera->GetCameraComponent()->SetReticleScreenPos(screenPos);
    }
}

void GameScene::Update(float deltatime)
{
    Input::Update();

    // --- レティクル入力を先に処理（ドラッグ） ---
    if (Input::IsMouseLeftPressed()) m_isDragging = true;
    if (m_isDragging && Input::IsMouseLeftDown()) {
        m_lastDragPos = Input::GetMousePosition();
        SetReticleByCenter(m_lastDragPos);
    }
    if (!Input::IsMouseLeftDown() && m_isDragging) {
        m_isDragging = false;
    }

    // 3) カメラにレティクル座標を渡す（最新のものを渡す）
    if (m_FollowCamera && m_FollowCamera->GetCameraComponent()) {
        m_FollowCamera->GetCameraComponent()->SetReticleScreenPos(Vector2((float)m_lastDragPos.x, (float)m_lastDragPos.y));
    }

    // 4) 全オブジェクト Update を一回だけ実行（重要）
    for (auto& obj : m_GameObjects)
    {
        if (obj) obj->Update(deltatime);
    }

    // 5) 衝突処理などは既存ロジック通り（下に続けてください）
    CollisionManager::Clear();
    for (auto& obj : m_GameObjects) {
        auto collider = obj->GetComponent<ColliderComponent>();
        if (collider) CollisionManager::RegisterCollider(collider.get());
    }
    CollisionManager::CheckCollisions();

    // ----------------- レティクルのドラッグ処理 -----------------
    // 押した瞬間にドラッグ開始
    if (Input::IsMouseLeftPressed())
    {
        m_isDragging = true;
    }

    // 押している間は追従
    if (m_isDragging /*&& Input::IsMouseLeftDown()*/)
    {
        m_lastDragPos = Input::GetMousePosition(); // GetMousePosition はクライアント座標を返す
        SetReticleByCenter(m_lastDragPos);
    }

    // 離したら固定してドラッグ終了
    if (!Input::IsMouseLeftDown() && m_isDragging)
    {
        m_isDragging = false;
        // m_lastDragPos を保持してその位置で止める
    }
    // ----------------------------------------------------------
   

    Vector3 ppos = m_player->GetPosition();
    //std::cout << "Playerのポジション : " << ppos.x << "," << ppos.y << "," << ppos.z << std::endl;
}

void GameScene::Draw(float deltatime)
{
    // 1) 通常のオブジェクト描画（既存）
    for (auto& obj : m_GameObjects)
    {
        if (obj) obj->Draw(deltatime);
    }

    // 2) CollisionManager に登録されているコライダーから、
    //    DebugRenderer にワイヤー箱を積む（色は IsHitThisFrame() を見て決定する）
    CollisionManager::DebugDrawAllColliders(gDebug);

    // 3) カメラから view/proj を取得（安全チェック）
    Matrix viewMat = Matrix::Identity;
    Matrix projMat = Matrix::Identity;

    if (m_FollowCamera && m_FollowCamera->GetCameraComponent())
    {
        viewMat = m_FollowCamera->GetCameraComponent()->GetView();
        projMat = m_FollowCamera->GetCameraComponent()->GetProj();
    }

    // 4) デバッグ線を常に見せたい場合は深度テストを OFF にする（ここでは既定でOFF）
    Renderer::SetDepthEnable(false);
    gDebug.Draw(viewMat, projMat);
    Renderer::SetDepthEnable(true);

    // 5) レティクルを HUD として描画
    //    m_reticleTex->GetSRV() で SRV を渡す。DrawReticle が深度/ブレンドを切り替えます。
    if (m_reticleTex && m_reticleTex->GetSRV())
    {
        Vector2 size(m_reticleW, m_reticleH);
        Renderer::DrawReticle(m_reticleTex->GetSRV(), m_lastDragPos, size);
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