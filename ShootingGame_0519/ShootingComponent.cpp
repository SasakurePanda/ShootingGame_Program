#include "ShootingComponent.h"
#include "Bullet.h"
#include "Input.h"
#include <Windows.h>
#include <iostream>
#include <DirectXMath.h>
#include <SimpleMath.h>
#include "Application.h"
#include "Enemy.h"
#include <algorithm>

using namespace DirectX::SimpleMath;

static bool GetMousePickRay(ICameraViewProvider* cam, const POINT& mousePos, SMS::Vector3& outNear, SMS::Vector3& outFar)
{
    //
    if (!cam)
    {
        std::cout << "GetMousePickRay: cam is null\n";
        return false;
    }

    float width  = Application::GetWidth();
    float height = Application::GetHeight();
    if (width <= 0 || height <= 0)
    {
        std::cout << "GetMousePickRay: invalid view size view=(" << width << "," << height << ")\n";
        return false;
    }

    // --- デバッグ: 画面中心テスト（中心に固定したときのレイとカメラ前方向の比較） ---
    {
        POINT centerMouse;
        centerMouse.x = width / 2;
        centerMouse.y = height / 2;

        SMS::Vector3 nearC, farC;
        bool okC = false;
        // 呼び出しは再帰を避けるため、ここでは内部で処理する本体ロジックを呼び出す形に分離する代わりに
        // 同一の処理をこのスコープ内で実行するために以下で内部計算を行う（下の本処理と同じ手順）。
        // ここではあくまでデバッグ用に中心点での変換結果を計算する。

        // View / Proj を取得
        SMS::Matrix view = cam->GetView();
        SMS::Matrix proj = cam->GetProj();

        // マウス中心 NDC 計算
        float fxC = static_cast<float>(centerMouse.x) + 0.5f;
        float fyC = static_cast<float>(centerMouse.y) + 0.5f;
        float xNC = (2.0f * fxC / static_cast<float>(width)) - 1.0f;
        float yNC = 1.0f - (2.0f * fyC / static_cast<float>(height));

        // クリップ空間点
        DirectX::XMVECTOR clipNearC = DirectX::XMVectorSet(xNC, yNC, 0.0f, 1.0f);
        DirectX::XMVECTOR clipFarC = DirectX::XMVectorSet(xNC, yNC, 1.0f, 1.0f);

        // 行列を XMMATRIX に変換
        DirectX::XMMATRIX xmViewC = DirectX::XMLoadFloat4x4(reinterpret_cast<const DirectX::XMFLOAT4X4*>(&view));
        DirectX::XMMATRIX xmProjC = DirectX::XMLoadFloat4x4(reinterpret_cast<const DirectX::XMFLOAT4X4*>(&proj));

        // Projection 逆行列でクリップ->ビュー
        DirectX::XMMATRIX invProjC = DirectX::XMMatrixInverse(nullptr, xmProjC);
        DirectX::XMVECTOR viewNearH_C = DirectX::XMVector4Transform(clipNearC, invProjC);
        DirectX::XMVECTOR viewFarH_C = DirectX::XMVector4Transform(clipFarC, invProjC);

        float wvNearC = DirectX::XMVectorGetW(viewNearH_C);
        float wvFarC = DirectX::XMVectorGetW(viewFarH_C);
        if (fabsf(wvNearC) > 1e-6f && fabsf(wvFarC) > 1e-6f)
        {
            viewNearH_C = DirectX::XMVectorScale(viewNearH_C, 1.0f / wvNearC);
            viewFarH_C = DirectX::XMVectorScale(viewFarH_C, 1.0f / wvFarC);

            // View 逆行列でビュー->ワールド
            DirectX::XMMATRIX invViewC = DirectX::XMMatrixInverse(nullptr, xmViewC);
            DirectX::XMVECTOR worldNearC = DirectX::XMVector3TransformCoord(viewNearH_C, invViewC);
            DirectX::XMVECTOR worldFarC = DirectX::XMVector3TransformCoord(viewFarH_C, invViewC);

            DirectX::XMFLOAT3 nearF3C, farF3C;
            DirectX::XMStoreFloat3(&nearF3C, worldNearC);
            DirectX::XMStoreFloat3(&farF3C, worldFarC);

            nearC = SMS::Vector3(nearF3C.x, nearF3C.y, nearF3C.z);
            farC = SMS::Vector3(farF3C.x, farF3C.y, farF3C.z);
            okC = true;
        }
        else
        {
            okC = false;
        }

        // 出力
        std::cout << "CenterTest: ok=" << (okC ? 1 : 0) << "\n";
        if (okC)
        {
            std::cout << "  center near=(" << nearC.x << "," << nearC.y << "," << nearC.z << ")"
                << " far=(" << farC.x << "," << farC.y << "," << farC.z << ")\n";

            SMS::Vector3 rayDirC = farC - nearC;
            float lenC = rayDirC.Length();
            if (lenC > 1e-6f) rayDirC /= lenC;
            SMS::Vector3 camF = cam->GetForward();
            float fLen = camF.Length();
            if (fLen > 1e-6f) camF /= fLen;

            float dotC;
            dotC = rayDirC.Dot(camF);
            dotC = std::clamp(dotC, -1.0f, 1.0f);
            float angleRadC = acosf(dotC);
            float angleDegC = angleRadC * 180.0f / 3.14159265358979323846f;

            std::cout << "  rayDir=(" << rayDirC.x << "," << rayDirC.y << "," << rayDirC.z << ") len=" << lenC << "\n";
            std::cout << "  camForward=(" << camF.x << "," << camF.y << "," << camF.z << ")\n";
            std::cout << "  dot=" << dotC << " angleDeg=" << angleDegC << "\n";

            if (angleDegC < 5.0f)
                std::cout << "  Center check: OK (ray ~ camera forward)\n";
            else
                std::cout << "  Center check: MISMATCH (View/Proj likely problematic)\n";
        }
    }
    // --- デバッグ終了 ---

    // --- 本来の処理: 指定 mousePos から near/far を計算する ---
    // View / Proj を取得
    SMS::Matrix view = cam->GetView();
    SMS::Matrix proj = cam->GetProj();

    // 入力座標とビューポート情報のログ
    std::cout << "Input mouse=(" << mousePos.x << "," << mousePos.y << ") view=(" << width << "," << height << ")\n";

    // ピクセル中心補正
    float fx = static_cast<float>(mousePos.x) + 0.5f;
    float fy = static_cast<float>(mousePos.y) + 0.5f;

    // NDC 変換
    float xN = (2.0f * fx / static_cast<float>(width)) - 1.0f;
    float yN = 1.0f - (2.0f * fy / static_cast<float>(height));
    std::cout << "  calc xN=" << xN << " yN=" << yN << "\n";

    // クリップ空間点（DirectX 標準: Z 0..1）
    DirectX::XMVECTOR clipNear = DirectX::XMVectorSet(xN, yN, 0.0f, 1.0f);
    DirectX::XMVECTOR clipFar = DirectX::XMVectorSet(xN, yN, 1.0f, 1.0f);

    // 行列を XMMATRIX に変換
    DirectX::XMMATRIX xmProj = DirectX::XMLoadFloat4x4(reinterpret_cast<const DirectX::XMFLOAT4X4*>(&proj));
    DirectX::XMMATRIX xmView = DirectX::XMLoadFloat4x4(reinterpret_cast<const DirectX::XMFLOAT4X4*>(&view));

    // Projection の逆行列で クリップ -> ビュー 空間へ
    DirectX::XMMATRIX invProj = DirectX::XMMatrixInverse(nullptr, xmProj);
    DirectX::XMVECTOR viewNearH = DirectX::XMVector4Transform(clipNear, invProj);
    DirectX::XMVECTOR viewFarH = DirectX::XMVector4Transform(clipFar, invProj);

    float wvNear = DirectX::XMVectorGetW(viewNearH);
    float wvFar = DirectX::XMVectorGetW(viewFarH);
    if (fabsf(wvNear) < 1e-6f || fabsf(wvFar) < 1e-6f)
    {
        std::cout << "GetMousePickRay: invalid w after invProj (wvNear=" << wvNear << " wvFar=" << wvFar << ")\n";
        return false;
    }
    viewNearH = DirectX::XMVectorScale(viewNearH, 1.0f / wvNear);
    viewFarH = DirectX::XMVectorScale(viewFarH, 1.0f / wvFar);

    // View の逆行列で ビュー -> ワールド
    DirectX::XMMATRIX invView = DirectX::XMMatrixInverse(nullptr, xmView);
    DirectX::XMVECTOR worldNear = DirectX::XMVector3TransformCoord(viewNearH, invView);
    DirectX::XMVECTOR worldFar = DirectX::XMVector3TransformCoord(viewFarH, invView);

    DirectX::XMFLOAT3 nearF3, farF3;
    DirectX::XMStoreFloat3(&nearF3, worldNear);
    DirectX::XMStoreFloat3(&farF3, worldFar);

    outNear = SMS::Vector3(nearF3.x, nearF3.y, nearF3.z);
    outFar = SMS::Vector3(farF3.x, farF3.y, farF3.z);

    std::cout << "Result near=(" << outNear.x << "," << outNear.y << "," << outNear.z << ")"
        << " far=(" << outFar.x << "," << outFar.y << "," << outFar.z << ")\n";

    return true;
}

static float DistancePointToRay(const Vector3& origin, const Vector3& dir, const Vector3& point)
{
    Vector3 v = point - origin;
    float t = v.Dot(dir); // プロジェクション長
    if (t < 0.0f) t = 0.0f; // レイは片方向
    Vector3 closest = origin + dir * t;
    return (point - closest).Length();
}

void ShootingComponent::Update(float dt)
{
    // タイマー更新
    m_timer += dt;

    // 所有者（Player）チェック
    GameObject* owner = GetOwner();
    if (!owner) return;

    // 発射トリガー（スペース押しで発射）
    // IsKeyDown を使うと押しっぱなしで連射（cooldown が効く）になります
    bool wantFire = Input::IsKeyDown(VK_SPACE);
    if (!wantFire) return;

    // クールダウン管理
    if (m_timer < m_cooldown) { return; }

    Vector3 aimDir = Vector3::Forward;
    bool haveAimDir = false;

    if (m_camera)
    {
        Vector3 aimPoint = m_camera->GetAimPoint();
        Vector3 spawnBase;
        Vector3 forward = owner->GetForward();

        if (forward.LengthSquared() > 1e-6f)
        {
            forward.Normalize();
            spawnBase = owner->GetPosition() + forward * m_spawnOffset;
        }
        else
        {
            // フォールバック（ワールド -Z 方向へオフセット）
            spawnBase = owner->GetPosition() + Vector3(0.0f, 0.0f, -m_spawnOffset);
        }

        Vector3 dirToAim = aimPoint - spawnBase;
        if (dirToAim.LengthSquared() > 1e-6f)
        {
            dirToAim.Normalize();
            aimDir = dirToAim;
            haveAimDir = true;
        }
    }

    if (!haveAimDir && m_camera)
    {
        // フォールバック: カメラから直接レティクル方向の単位ベクトルを取得
        Vector3 d = m_camera->GetAimDirectionFromReticle();
        if (d.LengthSquared() > 1e-6f)
        {
            d.Normalize();
            aimDir = d;
            haveAimDir = true;
        }
    }

    if (!haveAimDir)
    {
        // 最悪オーナーの前方を使う
        Vector3 ofwd = owner->GetForward();
        if (ofwd.LengthSquared() > 1e-6f)
        {
            ofwd.Normalize();
            aimDir = ofwd;
        }
        else
        {
            aimDir = Vector3::Forward;
        }
    }

    // 2) 発射位置（プレイヤーの少し前）
    Vector3 ownerFwd = owner->GetForward();
    if (ownerFwd.LengthSquared() < 1e-6f)
        ownerFwd = Vector3(0, 0, 1); // 安全策
    else
        ownerFwd.Normalize();

    Vector3 spawnPos = owner->GetPosition() + ownerFwd * m_spawnOffset;

    // 3) 弾の生成・初期化
    auto bullet = std::make_shared<Bullet>();
    bullet->SetPosition(spawnPos);
    bullet->Initialize();

    // 4) BulletComponent の設定（速度・向き・所有者フラグ等）
    if (auto bc = bullet->GetComponent<BulletComponent>())
    {
        bc->SetVelocity(aimDir); // 方向（normalized 期待）
        bc->SetSpeed(m_bulletSpeed);
        bc->SetBulletType(BulletComponent::BulletType::PLAYER);
        // 必要なら寿命やホーミング設定もここで行える：
        // bc->SetLifetime(4.0f);
        // bc->SetHomingStrength(0.0f);
    }

    // 5) シーン登録（m_scene がセットされていることを期待）
    if (m_scene)
    {
        m_scene->AddObject(bullet);
    }
    else
    {
        // フォールバック: 所有者のシーンに登録
        if (owner->GetScene())
        {
            owner->GetScene()->AddObject(bullet);
        }
        else
        {
            // どこにも登録できないケースはログ出し
            OutputDebugStringA("ShootingComponent::Update - no scene to add bullet\n");
        }
    }

    // タイマーをリセット（クールダウン開始）
    m_timer = 0.0f;
}

std::shared_ptr<GameObject> ShootingComponent::CreateBullet(const Vector3& pos, const Vector3& dir)
{
    //GameObjectを継承したBulletを生成
    auto bullet = std::make_shared<Bullet>();

    //初期位置・回転
    bullet->SetPosition(pos);
    //OutputDebugStringA("CreateBullet: after SetPosition\n");

    //Initializeしてコンポーネントを構築（Bullet::Initialize 内で Component を追加している想定）
    bullet->Initialize();
    //OutputDebugStringA("CreateBullet: after Initialize\n");

    //BulletComponentを取得して初期速度を与える
    auto bc = bullet->GetComponent<BulletComponent>();

    //BulletComponentクラスがあったら
    if (bc)
    {
        Vector3 d = dir;
        if (d.LengthSquared() < 1e-6f)
        {
            d = Vector3::UnitZ;
        }
        d.Normalize();
        bc->SetVelocity(d);
        bc->SetSpeed(m_bulletSpeed);

    }
    return bullet;
}

/*// --- 2) Xキーでのロックオンモード ---
    bool xNow = Input::IsKeyDown('X');
    if (xNow && m_canSelect && m_selectedTargets.size() < static_cast<size_t>(m_maxTargets))
    {
        //マウスのポジションを取ってくる
        POINT mpos = Input::GetMousePosition();

        //ニアファー準備
        Vector3 nearW, farW;
        if (m_camera && GetMousePickRay(m_camera, mpos, nearW, farW))
        {
            //レイのStartをニア(カメラに近い点)
            Vector3 rayOrigin = nearW;
            Vector3 rayDir = farW - nearW;
            if (rayDir.LengthSquared() > 1e-6f)
            {
                rayDir.Normalize();
            }
            else
            {
                rayDir = Vector3::UnitZ;
            }

            if (m_scene)
            {
                const auto& objs = m_scene->GetObjects();
                for (const auto& sp : objs)
                {
                    if (!sp)
                    {
                        continue;
                    }

                    Enemy* enemyPtr = dynamic_cast<Enemy*>(sp.get());

                    if (!enemyPtr)
                    {
                        continue;
                    }

                    // 既に選択済みかチェック
                    bool already = false;
                    for (auto& w : m_selectedTargets)
                    {
                        if (auto spt = w.lock())
                        {
                            if (spt.get() == sp.get()) { already = true; break; }
                        }
                    }
                    if (already) continue;

                    // レイと敵中心の最近接距離で判定
                    Vector3 enemyPos = sp->GetPosition();
                    float dist = DistancePointToRay(rayOrigin, rayDir, enemyPos);
                    if (dist <= m_selectionRadiusWorld)
                    {
                        m_selectedTargets.push_back(sp);
                        if (m_selectedTargets.size() >= static_cast<size_t>(m_maxTargets)) break;
                    }
                }
            }
        }
    }

    // X を離した瞬間にロックオン弾を発射
    if (m_xWasDown && !xNow)
    {
        if (!m_selectedTargets.empty())
        {
            Vector3 spawnPos = owner->GetPosition() + forwardDir * m_spawnOffset;

            for (auto& weakT : m_selectedTargets)
            {
                auto targetSp = weakT.lock();
                if (!targetSp) continue;
                Vector3 targetPos = targetSp->GetPosition();

                Vector3 dir = targetPos - spawnPos;
                if (dir.LengthSquared() < 1e-6f) dir = forwardDir;
                else dir.Normalize();

                auto bulletObj = CreateBullet(spawnPos, dir);
                if (!bulletObj) continue;

                auto bc = bulletObj->GetComponent<BulletComponent>();
                if (bc)
                {
                    bc->SetBulletType(BulletComponent::BulletType::PLAYER);
                    bc->SetTarget(targetSp);
                    bc->SetHomingStrength(m_homingStrength);
                    bc->SetSpeed(m_bulletSpeed);
                }

                if (m_scene) m_scene->AddObject(bulletObj);
            }

            // 発射後リセット＆クールダウン
            m_selectedTargets.clear();
            m_canSelect = false;
            m_selectionTimer = 0.0f;
        }
    }

    // フラグ更新
    m_xWasDown = xNow;*/


/*// タイマー更新
    m_timer += dt;
    if (!GetOwner())
    {
        return;
    }

    // シーンやカメラが無ければ何もしない（安全策）
    if (!m_scene || !m_camera)
    {
        return;
    }


    // ---- いまのキー状態を取得（毎フレーム） ----
    bool spaceNow = Input::IsKeyDown(VK_SPACE);        // 現在スペースが押されているか
    bool rightNow = Input::IsMouseRightDown();        // 現在右クリックが押されているか







    // --- 1) ホーミング「選択」処理（右クリック + SPACE を押している間に選択追加） ---
    //    （注）選択のロジックはクールダウンに依存させるべきではないが、現状の流れに合わせ最小変更。
    if (rightNow)
    {
        // まずマウス位置とカメラの行列を取得
        POINT mp = Input::GetMousePosition();
        Matrix view = m_camera->GetView();
        Matrix proj = m_camera->GetProj();

        // スクリーン座標 -> ワールドレイ（origin, dir）を作る
        Vector3 rayOrigin, rayDir;
        ScreenToWorld((float)mp.x, (float)mp.y, view, proj, rayOrigin, rayDir);

        // シーン中のオブジェクト群を取得（IScene::GetObjects がある前提）
        const auto& objs = m_scene->GetObjects();

        for (const auto& o : objs)
        {
            if (!o) continue;

            // 敵だけを対象にする（Enemy クラスなら続行）
            auto enemySp = std::dynamic_pointer_cast<class Enemy>(o);
            if (!enemySp) continue;

            // 既に選択済みならスキップ（IsAlreadySelected は後述）
            if (IsAlreadySelected(m_selectedTargets, o)) continue;

            // 敵の中心（またはバウンディングセンター）を取得してレイとの距離を測る
            Vector3 center = enemySp->GetPosition();

            // DistanceSquared_PointToRay(center, rayOrigin, rayDir, m_rayMaxDist)
            // を使って、点とレイの最短距離の二乗を取得する（m_rayMaxDist は ray の最大長）
            float distSq = DistanceSquared_PointToRay(center, rayOrigin, rayDir, m_rayMaxDist);

            // m_selectionRadiusWorld はワールド単位の半径（メンバ変数）
            if (distSq <= (m_selectionRadiusWorld * m_selectionRadiusWorld))
            {
                // 選択追加（m_selectedTargets は std::vector<std::weak_ptr<GameObject>> を想定）
                m_selectedTargets.push_back(o);
                std::cout << "[Shooting] Selected target at ("
                    << center.x << "," << center.y << "," << center.z << ")\n";

                // 上限に達したらループ抜け
                if ((int)m_selectedTargets.size() >= m_maxTargets) break;
            }
        }
    }

    // --- 2) 「スペースのリリース」を検出して、かつ右クリックが押されている（まだ）なら選択済みターゲットへ一斉発射 ---
    // ここで m_spaceWasDown はメンバ変数（前フレームのスペース状態）で、関数末尾で更新する
    if (Input::IsMouseRightPressed())
    {
        // 発射はクールダウン満たしている場合のみ行う（安全）
        if (m_timer >= m_cooldown && !m_selectedTargets.empty())
        {
            // 発射位置：オーナー（プレイヤー）位置 + カメラ前方オフセット
            Vector3 camForward = m_camera->GetForward();
            if (camForward.LengthSquared() < 1e-6f) camForward = Vector3::UnitZ;
            camForward.Normalize();
            Vector3 spawnPos = GetOwner()->GetPosition() + camForward * m_spawnOffset;

            // 選択されたターゲットそれぞれへ弾を発射
            for (auto& weakT : m_selectedTargets)
            {
                auto targetSp = weakT.lock();
                if (!targetSp) continue;

                // 発射方向はターゲットに向かうベクトル
                Vector3 toTarget = targetSp->GetPosition() - spawnPos;
                if (toTarget.LengthSquared() < 1e-6f) toTarget = camForward;
                toTarget.Normalize();

                // 弾を作る
                auto bulletObj = CreateBullet(spawnPos, toTarget);
                if (bulletObj)
                {
                    // 弾の BulletComponent を取得してターゲットを渡す（weak_ptr を受け取れる実装想定）
                    if (auto bc = bulletObj->GetComponent<BulletComponent>())
                    {
                        bc->SetBulletType(BulletComponent::BulletType::PLAYER);
                        bc->SetTarget(targetSp);          // SetTarget で weak_ptr を受け取れること
                        bc->SetHomingStrength(8.0f);      // 調整値
                        bc->SetSpeed(m_bulletSpeed);
                    }
                    m_scene->AddObject(bulletObj);
                }
            }

            std::cout << "[Shooting] Fired homing bullets: count=" << m_selectedTargets.size() << "\n";

            // 発射後：選択解除＆クールダウンスタート
            m_selectedTargets.clear();
            m_timer = 0.0f;
        }
    }

    // --- 3) 右クリックを押していない通常発射（スペース長押しの連射） ---
    if (spaceNow && m_timer >= m_cooldown)
    {
        // 1) マウス位置（クライアント座標）を取る
        POINT mp = Input::GetMousePosition();

        // 2) カメラの行列
        Matrix view = m_camera->GetView();
        Matrix proj = m_camera->GetProj();

        // 3) スクリーン座標 -> ワールドのレイ (origin, dir) を作る
        Vector3 rayOrigin, rayDir;
        // ScreenPointToWorldRay: 下に実装例を載せている関数を使う想定
        ScreenToWorld((float)mp.x, (float)mp.y, view, proj, rayOrigin, rayDir);

        // 4) 発射方向はレイ方向（正規化済み）を使う
        Vector3 dir = rayDir;
        if (dir.LengthSquared() < 1e-6f)
        {
            dir = m_camera->GetForward();
        }// フォールバック

        dir.Normalize();

        // 5) 発射位置：プレイヤーの位置 + dir * m_spawnOffset（安全で自然）
        Vector3 spawnPos = GetOwner()->GetPosition() + dir * m_spawnOffset;

        // 6) 弾を生成して設定（CreateBullet は既存の関数を利用）
        auto bulletObj = CreateBullet(spawnPos, dir);
        if (bulletObj)
        {
            if (auto bc = bulletObj->GetComponent<BulletComponent>())
            {
                bc->SetBulletType(BulletComponent::BulletType::PLAYER);
                bc->SetSpeed(m_bulletSpeed);
            }
            m_scene->AddObject(bulletObj);
            std::cout << "[Shooting] Spawn NORMAL bullet (towards mouse) at ("
                << spawnPos.x << "," << spawnPos.y << "," << spawnPos.z << ")\n";
        }

        // リセット
        m_timer = 0.0f;
    }

    // --- 最後に前フレームのスペース状態を保存（リリース検出のため） ---
    m_spaceWasDown = spaceNow;*/


/*void ShootingComponent::Update(float dt)
{
    m_timer += dt;
    if (!GetOwner()) 
    {
        return;
    }

    // 選択クールダウンの更新
    if (!m_canSelect) //選択が禁止中（クールダウン中）なら
    {
        m_selectionTimer -= dt; // クールタイマを進める
        if (m_selectionTimer <= 0.0f) // クールタイムが終わったら
        {
            m_canSelect = true; // 選択可能フラグを戻す
            m_selectionTimer = 0.0f; // タイマをクリア
        }
    }

    //選択処理(左クリック押した瞬間に選択)
    if (m_canSelect && Input::IsMouseLeftDown())
    {
        if (!m_scene || !m_camera)
        {
            std::cout << "[Shooting] Scene or camera missing, can't select\n";
        }
        else
        {
            //マウスの位置(クライアント座標)
            POINT mp = Input::GetMousePosition();

            //カメラの行列の取得
            Matrix view = m_camera->GetView();
            Matrix proj = m_camera->GetProj();

            ////ロックオンに使用する為に飛ばすレイを作る
            //Vector3 rayOrigin, rayDir;
            ////マウスのX、マウスのY、ビュー行列、プロジェクト行列
            //ScreenToWorld(sx, sy, view, proj, rayOrigin, rayDir);

            //選択しているかどうかを調べるために
            //Sceneのオブジェクトを取得する
            const auto& objs = m_scene->GetObjects();

            //走査して候補を集める
            //(ここではスクリーン距離で簡易選択)
            for (const auto& o : objs)
            {
                if (!o)  //nullならStop
                {
                    continue; 
                }

                // 敵のみを対象にするチェック（型判定 or tag）
                // ここでは dynamic_cast 例（実装次第で変える）
                auto enemySp = std::dynamic_pointer_cast<class Enemy>(o); // Enemy へキャスト
                if (!enemySp)//敵でなければスキップ
                {
                    continue;
                } 

                //既に選択済みかどうかの検査
                bool already = false; //フラグ初期化
                for (auto& w : m_selectedTargets) //現在選択中のリストを検査
                {
                    if (auto s = w.lock()) //weak_ptrをロックしてshared_ptrを取得
                    {
                        if (s.get() == o.get()) //同一オブジェクトかチェック
                        {
                            already = true; //既に選択済み
                            break; //ループ抜け
                        }
                    }
                }


                if (already)
                {
                    continue; 
                }

                // World -> Screen
                Vector3 screenPos = ProjectWorldToScreenPoint(enemySp->GetPosition(), view, proj); //ワールド->スクリーン

                float dx = screenPos.x - static_cast<float>(mp.x);
                float dy = screenPos.y - static_cast<float>(mp.y);
                // 許容ピクセル半径内なら選択とみなす（m_selectionRadiusPx を使う）
                if (dx * dx + dy * dy <= m_selectionRadiusPx * m_selectionRadiusPx) // 半径以内か？
                {
                    m_selectedTargets.push_back(o); // 選択リストに追加（weak_ptr に変換される）
                    std::cout << "[Shooting] Selected target at ("
                        << enemySp->GetPosition().x << ","
                        << enemySp->GetPosition().y << ","
                        << enemySp->GetPosition().z << ")\n"; // ログ出力
                    std::cout <<"ロックオン成功しました" << std::endl;

                    if ((int)m_selectedTargets.size() >= m_maxTargets) // 最大数に達したら
                        break; // 追加探索を打ち切る
                }
            }
        }
    }

    // -------- 発射処理 --------
    if (m_timer >= m_cooldown && Input::IsKeyDown(VK_SPACE)) // クールダウンが終わりかつ SPACE 押下中なら発射
    {
        if (!m_scene || !m_camera) // シーンやカメラがない場合は何もしない（安全策）
        {
            m_timer = 0.0f; // タイマをリセットしておく
            return; // 早期リターン
        }

        // プレイヤー（オーナー）位置を取り出す
        Vector3 ownerPos = GetOwner()->GetPosition(); // 自身の座標

        // 通常はカメラの forward を使って発射方向を決める（まずフォールバックを考慮）
        Vector3 camForward = m_camera->GetForward(); // カメラの正面方向ベクトル
        if (camForward.LengthSquared() < 1e-6f) camForward = Vector3::UnitZ; // 無効ならデフォルト Z 軸
        camForward.Normalize(); // 正規化

        // 発射位置はオーナー位置 + カメラ前方へオフセット（銃口位置の簡易）
        Vector3 spawnPos = ownerPos + camForward * m_spawnOffset; // 発射スポーン位置

        // --- 右クリック押下時の特別処理: マウス方向へ撃つ（例: 照準合わせしたい時の動き） ---
        if (Input::IsMouseRightDown()) // 右ボタンを押している（ホールド中）
        {
            // 右クリック時はマウス位置のスクリーン座標から ray を作って、その方向へ撃つ（より直感的な狙い）
            POINT mpos = Input::GetMousePosition(); // マウス位置取得
            float sx = (float)mpos.x; // スクリーン X
            float sy = (float)mpos.y; // スクリーン Y

            // View / Projection を取得して DirectX のアンプロジェクトを使う
            Matrix view = m_camera->GetView(); // ビュー行列
            Matrix proj = m_camera->GetProj(); // 射影行列

            // XM 用に変換して near/far をアンプロジェクトする（近遠点でワールド座標を作る）
            XMMATRIX viewXM = XMLoadFloat4x4(reinterpret_cast<const XMFLOAT4X4*>(&view)); // XMLoad
            XMMATRIX projXM = XMLoadFloat4x4(reinterpret_cast<const XMFLOAT4X4*>(&proj)); // XMLoad
            XMMATRIX worldXM = XMMatrixIdentity(); // ワールドは単位行列

            // 画面の near/far 点をワールドに戻す（DirectX の Z は 0..1）
            XMVECTOR nearScreen = XMVectorSet(sx, sy, 0.0f, 1.0f); // near
            XMVECTOR farScreen = XMVectorSet(sx, sy, 1.0f, 1.0f); // far

            XMVECTOR nearWorldV = XMVector3Unproject(nearScreen, 0.0f, 0.0f,
                static_cast<float>(Application::GetWidth()), static_cast<float>(Application::GetHeight()),
                0.0f, 1.0f, projXM, viewXM, worldXM); // near をワールドへ

            XMVECTOR farWorldV = XMVector3Unproject(farScreen, 0.0f, 0.0f,
                static_cast<float>(Application::GetWidth()), static_cast<float>(Application::GetHeight()),
                0.0f, 1.0f, projXM, viewXM, worldXM); // far をワールドへ

            // XMVector から SimpleMath::Vector3 に変換
            Vector3 nearW((float)XMVectorGetX(nearWorldV), (float)XMVectorGetY(nearWorldV), (float)XMVectorGetZ(nearWorldV));
            Vector3 farW((float)XMVectorGetX(farWorldV), (float)XMVectorGetY(farWorldV), (float)XMVectorGetZ(farWorldV));

            // マウスの指す方向は near->far のベクトル
            Vector3 mouseDir = farW - nearW; // near->far ベクトル
            if (mouseDir.LengthSquared() < 1e-6f) mouseDir = camForward; // 極端に短ければカメラ前方を使う
            mouseDir.Normalize(); // 正規化

            // 弾生成（マウス目標方向）
            auto bulletObj = CreateBullet(spawnPos, mouseDir); // CreateBullet は GameObject (Bullet) を返すはず
            if (bulletObj) // 生成成功したら
            {
                auto bc = bulletObj->GetComponent<BulletComponent>(); // BulletComponent を取る
                if (bc) // 存在すればタイプや速度を設定
                {
                    bc->SetBulletType(BulletComponent::BulletType::PLAYER); // プレイヤー弾としてマーク
                    bc->SetSpeed(m_bulletSpeed * 1.2f); // 右クリック時は少し速くする例（調整可）
                }
                m_scene->AddObject(bulletObj); // シーンへ追加して発射を反映
                std::cout << "[Shooting] Spawn right-click aimed bullet\n"; // デバッグ出力
            }

            m_timer = 0.0f; // 発射後はクールダウンリセット
            return; // 右クリック処理をしたら早期に戻す（左クリックでの同時二重発射防止）
        } // if IsMouseRightDown

        // --- 選択ターゲットがある場合: 選択ターゲットそれぞれにホーミング弾を発射する ---
        if (!m_selectedTargets.empty()) // 選択されたターゲットがあるか？
        {
            for (auto& weakT : m_selectedTargets) // 各選択ターゲットについてループ
            {
                auto targetSp = weakT.lock(); // weak_ptr -> shared_ptr に変換
                if (!targetSp) continue; // 既に破棄されていたらスキップ

                // ターゲット方向を計算して弾を生成
                Vector3 toTarget = targetSp->GetPosition() - spawnPos; // ターゲットへ向かうベクトル
                if (toTarget.LengthSquared() < 1e-6f) toTarget = camForward; // 極端に小さければカメラ前方を使う
                toTarget.Normalize(); // 正規化

                auto bulletObj = CreateBullet(spawnPos, toTarget); // ホーミング弾生成
                if (bulletObj)
                {
                    auto bc = bulletObj->GetComponent<BulletComponent>(); // 弾コンポーネント取得
                    if (bc)
                    {
                        bc->SetBulletType(BulletComponent::BulletType::PLAYER); // プレイヤー弾
                        bc->SetTarget(targetSp); // ターゲットを渡してホーミングさせる（weak_ptr で保持される）
                        bc->SetHomingStrength(10.0f); // ホーミング強度（任意調整）
                        bc->SetSpeed(m_bulletSpeed); // 速度設定
                    }
                    m_scene->AddObject(bulletObj); // シーンに追加
                    std::cout << "[Shooting] Spawn homing bullet for one selected target\n"; // ログ
                }
            } // for selectedTargets

            // 発射後の後処理：選択解除と選択クールダウン
            ClearSelection(); // 選択解除
            m_canSelect = false; // 選択禁止にする
            m_selectionTimer = m_selectionCooldown; // クールダウンを設定
            m_timer = 0.0f; // 発射クールダウンリセット
            return; // 発射処理完了、早期リターン
        } // if selectedTargets not empty

        // --- 通常弾（何も選択しておらず右クリックも押していない）: カメラ前方へ単発 ---
        {
            auto bulletObj = CreateBullet(spawnPos, camForward); // カメラ前方を向いた弾を生成
            if (bulletObj) // 生成成功なら
            {
                auto bc = bulletObj->GetComponent<BulletComponent>(); // BulletComponent を取る
                if (bc)
                {
                    bc->SetBulletType(BulletComponent::BulletType::PLAYER); // プレイヤー弾に設定
                    bc->SetSpeed(m_bulletSpeed); // 速度を設定
                }
                m_scene->AddObject(bulletObj); // シーンに追加して実際に射出
                std::cout << "[Shooting] Spawn normal bullet (camera forward)\n"; // ログ
            }
            m_timer = 0.0f; // 発射後はクールダウンをリセット
            return; // 通常弾処理終了、戻る
        }
    } // if timer >= cooldown && SPACE
} // Update*/