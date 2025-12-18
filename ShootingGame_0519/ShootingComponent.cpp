#include "ShootingComponent.h"
#include "Bullet.h"
#include "Enemy.h"
#include "Input.h"
#include "GameObject.h"
#include "IScene.h"
#include "Application.h"

#include <iostream>
#include <DirectXMath.h>
using namespace DirectX;
using namespace DirectX::SimpleMath;

void ShootingComponent::FireHomingBullet(GameObject* owner, const std::shared_ptr<GameObject>& targetSp)
{
    // プレイヤーの前方向
    Vector3 forward = owner->GetForward();
    if (forward.LengthSquared() < 1e-6f)
    {
        forward = Vector3::UnitZ;
    }
    else
    {
        forward.Normalize();
    }

    //発射位置計算
    Vector3 muzzleLocal(0.0f, 0.0f, m_spawnOffset);
    Vector3 rot = owner->GetRotation();
    Matrix rotM = Matrix::CreateFromYawPitchRoll(rot.y, rot.x, rot.z);
    Vector3 spawnPos = owner->GetPosition() + Vector3::Transform(muzzleLocal, rotM);

    //最初は前方に撃つ
    Vector3 initialDir = forward;

    auto bullet = CreateBullet(spawnPos, initialDir, m_homingBulletColor, targetSp);
    if (bullet)
    {
        if (auto bc = bullet->GetComponent<BulletComponent>())
        {
            bc->SetVelocity(initialDir);
            bc->SetSpeed(m_bulletSpeed);
            bc->SetBulletType(BulletComponent::PLAYER);
            bc->SetHomingStrength(m_homingStrength);
            bc->SetTarget(targetSp);
        }
        AddBulletToScene(bullet);
    }

    //クールタイム
    m_timer = 0.0f;
}


std::shared_ptr<GameObject> ShootingComponent::FindBestHomingTarget()
{
    if(!m_scene || !m_camera) { return nullptr; }

    const auto& objects = m_scene->GetObjects();

    float screenW = static_cast<float>(Application::GetWidth());
    float screenH = static_cast<float>(Application::GetHeight());
    float cx = screenW * 0.5f;
    float cy = screenH * 0.5f;

    //画面中央周辺だけを対象にするための半径を設定
    float maxScreenRadius = 400.0f;

    Matrix view = m_camera->GetView();
    Matrix proj = m_camera->GetProj();
    XMMATRIX viewXM = XMLoadFloat4x4(reinterpret_cast<const XMFLOAT4X4*>(&view));
    XMMATRIX projXM = XMLoadFloat4x4(reinterpret_cast<const XMFLOAT4X4*>(&proj));
    XMMATRIX worldXM = XMMatrixIdentity();

    std::shared_ptr<GameObject> bestTarget;
    float bestScore = FLT_MAX;

    for (auto& obj : objects)
    {
        if (!obj) continue;

        if (!std::dynamic_pointer_cast<Enemy>(obj)) { continue; }

        Vector3 worldPos = obj->GetPosition();

        //ワールド座標をスクリーン座標に変換する
        XMVECTOR screenPos = XMVector3Project(XMLoadFloat3(reinterpret_cast<const XMFLOAT3*>(&worldPos)),
                                              0.0f, 0.0f, screenW, screenH, 0.0f, 1.0f,projXM, viewXM, worldXM);

        //0～1の範囲になら前にいる、1以上または負の数なら
        float sx = XMVectorGetX(screenPos);
        float sy = XMVectorGetY(screenPos);
        float sz = XMVectorGetZ(screenPos); 

        //カメラの前方にいないなら
        if (sz < 0.0f || sz > 1.0f) { continue; }

        //画面内かざっくりチェック
        if (sx < 0.0f || sx > screenW || sy < 0.0f || sy > screenH) { continue; }

        //中央からの距離でスコアをつける（中央に近いほど優先）
        float dx = sx - cx;
        float dy = sy - cy;
        float dist2 = dx * dx + dy * dy;

        if (dist2 > maxScreenRadius * maxScreenRadius) { continue; }

        if (dist2 < bestScore)
        {
            bestScore = dist2;
            bestTarget = obj;
        }
    }

    return bestTarget;
}

std::weak_ptr<GameObject> ShootingComponent::ChooseHomingTarget() const
{
    //選択中ターゲットから生きているもの
    for (const auto& w : m_selectedTargets)
    {
        if (!w.expired())
        {
            return w;
        }
    }

    // 見つからない
    return std::weak_ptr<GameObject>();
}


static bool GetMouseRayWorld(ICameraViewProvider* camera,
                             const POINT& mousePos,
                             Vector3& outDir)
{

    if (!camera)
    {
        return false;
    }

    float width  = Application::GetWidth();
    float height = Application::GetHeight();

	//std::cout << "画面サイズ: " << width << "x" << height << "\n";

    if (width <= 0.0f || height <= 0.0f)
    {
        return false;
    }

    //カメラのビューとプロジェクトを取得
    Matrix view = camera->GetView();
    Matrix proj = camera->GetProj();

    //画面左上が(0,0)右下が(width , proj)
    float fx = static_cast<float>(mousePos.x) + 0.5f;
    float fy = static_cast<float>(mousePos.y) + 0.5f;

    float xN = (2.0f * fx / width) - 1.0f;
    float yN = 1.0f - (2.0f * fy / height);

    //std::cout << "NDC :" << xN << ", " << yN << "\n";

    XMVECTOR clipNear = XMVectorSet(xN, yN, 0.0f, 1.0f);
    XMVECTOR clipFar  = XMVectorSet(xN, yN, 1.0f, 1.0f);

    XMMATRIX xmProj = XMLoadFloat4x4(reinterpret_cast<const XMFLOAT4X4*>(&proj));
    XMMATRIX xmView = XMLoadFloat4x4(reinterpret_cast<const XMFLOAT4X4*>(&view));

    //クリップ空間からビュー空間に変換
    XMMATRIX invProj   = XMMatrixInverse(nullptr, xmProj);
    XMVECTOR viewNearH = XMVector4Transform(clipNear, invProj);
    XMVECTOR viewFarH = XMVector4Transform(clipFar , invProj);

    float wNear = XMVectorGetW(viewNearH);
    float wFar  = XMVectorGetW(viewFarH);

    if (fabsf(wNear) < 1e-6f || fabsf(wFar) < 1e-6f)
    {
        return false;
    }

    viewNearH = XMVectorScale(viewNearH, 1.0f / wNear);
    viewFarH = XMVectorScale(viewFarH, 1.0f / wFar);

    //ビュー空間からワールド空間
    XMMATRIX invView = XMMatrixInverse(nullptr, xmView);
    XMVECTOR worldNear = XMVector3TransformCoord(viewNearH, invView);
    XMVECTOR worldFar = XMVector3TransformCoord(viewFarH, invView);

    //レイの方向を正規化する
    XMVECTOR dir = XMVector3Normalize(worldFar - worldNear);
    XMFLOAT3 df;
    XMStoreFloat3(&df, dir);

    outDir = Vector3(df.x, df.y, df.z);
    return true;

}

void ShootingComponent::Update(float dt)
{
    // 経過時間更新
    m_timer += dt;

    // オーナー取得（Player 想定）
    GameObject* owner = GetOwner();
    if (!owner) { return; }

    //-------------ホーミング弾処理開始-----------------
    bool cDown = Input::IsKeyDown('C');   // もしくは VK_C でもOK
    bool justPressedC = (cDown && !m_prevHomingKeyDown);
    m_prevHomingKeyDown = cDown;

    if (justPressedC)
    {
        //クールタイムが終わっているか
        if (m_timer >= m_cooldown)
        {
            //画面上からターゲットを探す（実装済みを想定）
            std::shared_ptr<GameObject> targetSp = FindBestHomingTarget();
            if (targetSp)
            {
                // --- プレイヤー前方・発射位置計算 ---
                Vector3 forward = owner->GetForward();
                if (forward.LengthSquared() < 1e-6f)
                    forward = Vector3::UnitZ;
                else
                    forward.Normalize();

                //発射位置
                Vector3 muzzleLocal(0.0f, 0.0f, m_spawnOffset);
                Vector3 rot = owner->GetRotation();
                Matrix rotM = Matrix::CreateFromYawPitchRoll(rot.y, rot.x, rot.z);
                Vector3 spawnPos = owner->GetPosition() + Vector3::Transform(muzzleLocal, rotM);

                //初期方向：ターゲットの現在位置へまっすぐ
                Vector3 toTarget = targetSp->GetPosition() - spawnPos;
                if (toTarget.LengthSquared() < 1e-6f)
                    toTarget = forward;
                toTarget.Normalize();

                //弾生成
                auto bullet = CreateBullet(spawnPos, toTarget);
                if (bullet)
                {
                    if (auto bc = bullet->GetComponent<BulletComponent>())
                    {
                        bc->SetVelocity(toTarget);
                        bc->SetSpeed(750);

                        bc->SetTarget(targetSp);                 //追尾相手をセット
                        bc->SetHomingStrength(m_homingStrength); //追尾の強さ
                        bc->SetBulletType(BulletComponent::PLAYER);
                    }
                    AddBulletToScene(bullet);
                }

                //クールタイムリセット
                m_timer = 0.0f;

                // Cで撃ったときはここで return してもOK（SPACE射撃と混ぜたくない場合）
                return;
            }
        }
    }

    //発射入力（SPACE 押しっぱなしで連射）
    bool wantFire = m_autoFire || Input::IsKeyDown(VK_SPACE);
    if (!wantFire) { return; }

    //クールタイム未経過なら撃たない
    if (m_timer < m_cooldown) { return; }

    //----------カメラが無いとどうにもならないのでフォールバック-----------
	//-------------カメラが無い場合はオーナー前方に撃つだけ----------------
    if (!m_camera)
    {
        
        Vector3 forward = owner->GetForward();
        if (forward.LengthSquared() < 1e-6f)
        {
            forward = Vector3::UnitZ;
        }
        else 
        {
            forward.Normalize();
        }

        Vector3 muzzleLocal(0.0f, 0.0f, m_spawnOffset);
        Vector3 rot = owner->GetRotation();
        Matrix  rotM = Matrix::CreateFromYawPitchRoll(rot.y, rot.x, rot.z);
        Vector3 spawnPos = owner->GetPosition() + Vector3::Transform(muzzleLocal, rotM);

        auto bullet = CreateBullet(spawnPos, forward, m_normalBulletColor);
        if (bullet)
        {
            if (auto bc = bullet->GetComponent<BulletComponent>())
            {
                bc->SetVelocity(forward);
                bc->SetSpeed(m_bulletSpeed);
            }
            AddBulletToScene(bullet);
        }

        m_timer = 0.0f;
        return;
    }

    //----------ここから本命：カメラレイ上に「プレイヤー前に近い点」を取る----------

    //カメラ位置 & レティクル方向（Vテスト弾と同じ）
    Vector3 camPos = m_camera->GetPosition();
    Vector3 camDir = m_camera->GetAimDirectionFromReticle();
    if (camDir.LengthSquared() < 1e-6f)
    {
        camDir = m_camera->GetForward();
    }
    camDir.Normalize();

    //「プレイヤーの前あたり」の目安位置（マズル近く）
    Vector3 ownerForward = owner->GetForward();
    if (ownerForward.LengthSquared() < 1e-6f)
    {
        ownerForward = Vector3::UnitZ;
    }
    else
    {
        ownerForward.Normalize();
    }

    // プレイヤーの前方 m_spawnOffset の位置を「銃口の目安」とする
    Vector3 muzzleGuess = owner->GetPosition() + ownerForward * m_spawnOffset;

    // 3) カメラレイ上で muzzleGuess に一番近い点を求める
    Vector3 camToMuzzle = muzzleGuess - camPos;
    //レイ上の最近接点
    float t = camToMuzzle.Dot(camDir);

    // カメラのすぐ近くや後ろになるのを防ぐため、ある程度前方にクランプ
    if (t < 1.0f)
    {
        t = 1.0f;
    }

    //この点が実際の弾の出発位置（カメラレイ上なので、レティクル線と完全一致）
    Vector3 spawnPos = camPos + camDir * t;

    //方向は V 弾と同じく camDir
    Vector3 aimDir = camDir;

    //弾生成
    auto bullet = CreateBullet(spawnPos, aimDir, m_normalBulletColor);
    if (bullet)
    {
        if (auto bc = bullet->GetComponent<BulletComponent>())
        {
            bc->SetVelocity(aimDir);
            bc->SetSpeed(m_bulletSpeed);
        }
        AddBulletToScene(bullet);
    }

    //クールタイムリセット
    m_timer = 0.0f;

}

std::shared_ptr<GameObject> ShootingComponent::CreateBullet(const Vector3& pos,const Vector3& dir,
                                                            const Vector4& color, std::weak_ptr<GameObject> target)
{
    // GameObject を継承した Bullet を生成
    auto bullet = std::make_shared<Bullet>();

    // 初期位置
    bullet->SetPosition(pos);

    // Initialize（中でコンポーネント追加している想定）
    bullet->Initialize();

    // BulletComponent を取得して初期速度を与える
    auto bc = bullet->GetComponent<BulletComponent>();
    if (bc)
    {
        Vector3 d = dir;
        if (d.LengthSquared() < 1e-6f)
        {
            d = Vector3::UnitZ;
        }
        d.Normalize();

        bc->SetVelocity(d);                     // 方向（正規化）
        bc->SetSpeed(m_bulletSpeed);            // 速さ
        bc->SetBulletType(BulletComponent::PLAYER);
        bc->SetColor(color);                    // ★ 色を渡す

        //ターゲットがある場合はホーミング設定
        if (!target.expired())
        {
            bc->SetTarget(target);
            bc->SetHomingStrength(m_homingStrength);
        }
    }

    return bullet;
}

// ----------------------
// シーンに弾を追加
// ----------------------
void ShootingComponent::AddBulletToScene(const std::shared_ptr<GameObject>& bullet)
{
    if (m_scene)
    {
        m_scene->AddObject(bullet);
        return;
    }

    // ShootingComponent のオーナーが属しているシーンに追加
    if (auto owner = GetOwner())
    {
        if (owner->GetScene())
        {
            owner->GetScene()->AddObject(bullet);
        }
    }
}



