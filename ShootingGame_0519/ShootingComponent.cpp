#include "ShootingComponent.h"
#include "Bullet.h"
#include "Enemy.h"
#include "Input.h"
#include "GameObject.h"
#include "IScene.h"
#include "Application.h"
#include "HomingComponent.h"
#include <iostream>
#include <random>
#include <DirectXMath.h>

using namespace DirectX;
using namespace DirectX::SimpleMath;

/// <summary>
/// ホーミング弾を生成し、ターゲットをセットする関数
/// </summary>
/// <param name="owner">発射元のオブジェクト</param>
/// <param name="targetSp">ターゲットのポインタ</param>
void ShootingComponent::FireHomingBullet(GameObject* owner, const std::shared_ptr<GameObject>& targetSp)
{
    if (!owner) { return; }

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

    Vector3 toTarget = targetSp->GetPosition() - spawnPos;

    if (toTarget.LengthSquared() > 1e-6f)
    {
        toTarget.Normalize();
    }
    else
    {
        // もし近すぎるときはオーナー前方を使う
        toTarget = forward; // forward は owner->GetForward() 正規化済み
    }

    auto bullet = CreateBullet(spawnPos, toTarget, m_homingBulletColor);
    if (bullet)
    {
        if (auto bc = bullet->GetComponent<BulletComponent>())
        {
            bc->SetVelocity(toTarget);        // ここで必ずターゲット方向をセット
            bc->SetSpeed(m_bulletSpeed);      // 必要ならホーミング用の速度にする
            bc->SetBulletType(BulletComponent::PLAYER);

        }
    }

     auto homing = bullet->AddComponent<HomingComponent>();
     if (homing)
     {
         homing->SetTarget(targetSp);
         homing->SetTimeToIntercept(1.0f);    
         homing->SetMaxAcceleration(2000.0f); 
         homing->SetLifeTime(5.0f);

         // 乱数準備（static にしてコストを抑える）
         static thread_local std::mt19937 s_rng((unsigned)std::random_device{}());
         std::uniform_real_distribution<float> dist(-1.0f, 1.0f);

         // 発射元のローカル軸（右・上）を取得（rotM を使って安定的に作る）
         Vector3 right = Vector3::Transform(Vector3::UnitX, rotM);
         Vector3 up = Vector3::Transform(Vector3::UnitY, rotM);

         // ランダムベクトル：左右と上下方向に少しずつずらす（上下は控えめ）
         Vector3 randVec = right * dist(s_rng) + up * (dist(s_rng) * 0.6f);

         if (randVec.LengthSquared() < 1e-6f)
         {
             // フォールバック（稀にゼロになる対策）
             randVec = Vector3::UnitZ;
         }
         randVec.Normalize();

         float biasStrength = 0.25f; 
         float biasDecay = 0.25f;     

         homing->SetAimBias(randVec);
         homing->SetAimBiasStrength(biasStrength);
         homing->SetAimBiasDecay(biasDecay);
     }

    AddBulletToScene(bullet);

    m_timer = 0.0f;
}

/// <summary>
/// 飛ばす先のターゲットを探す関数
/// </summary>
/// <returns>見つかったターゲット、又はnullptr</returns>
std::shared_ptr<GameObject> ShootingComponent::FindBestHomingTarget()
{
    if (!m_scene || !m_camera) { return nullptr; }

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
        if (!obj) { continue; }

        if (!std::dynamic_pointer_cast<Enemy>(obj)) { continue; }

        Vector3 worldPos = obj->GetPosition();

        //ワールド座標をスクリーン座標に変換する
        XMVECTOR screenPos = XMVector3Project(XMLoadFloat3(reinterpret_cast<const XMFLOAT3*>(&worldPos)),
            0.0f, 0.0f, screenW, screenH, 0.0f, 1.0f, projXM, viewXM, worldXM);

        float sx = XMVectorGetX(screenPos);
        float sy = XMVectorGetY(screenPos);
        float sz = XMVectorGetZ(screenPos);

        // カメラの前方にいないなら除外
        if (sz < 0.0f || sz > 1.0f) { continue; }

        // 画面内にいないなら除外（ざっくり）
        if (sx < 0.0f || sx > screenW || sy < 0.0f || sy > screenH) { continue; }

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

/// <summary>
/// スクリーン上のマウス座標からカメラ方向のワールド空間方向ベクトルに変換する関数
/// </summary>
/// <param name="camera">カメラインターフェース</param>
/// <param name="mousePos">カメラのスクリーン座標</param>
/// <param name="outDir">ワールド座標を格納する参照引数</param>
/// <returns>変換できたかどうかのbool値</returns>
static bool GetMouseRayWorld(ICameraViewProvider* camera,
                             const POINT& mousePos,
                             Vector3& outDir)
{
    if (!camera) { return false; }

    float width = Application::GetWidth();
    float height = Application::GetHeight();

    if (width <= 0.0f || height <= 0.0f) { return false; }

    Matrix view = camera->GetView();
    Matrix proj = camera->GetProj();

    float fx = static_cast<float>(mousePos.x) + 0.5f;
    float fy = static_cast<float>(mousePos.y) + 0.5f;

    float xN = (2.0f * fx / width) - 1.0f;
    float yN = 1.0f - (2.0f * fy / height);

    XMVECTOR clipNear = XMVectorSet(xN, yN, 0.0f, 1.0f);
    XMVECTOR clipFar = XMVectorSet(xN, yN, 1.0f, 1.0f);

    XMMATRIX xmProj = XMLoadFloat4x4(reinterpret_cast<const XMFLOAT4X4*>(&proj));
    XMMATRIX xmView = XMLoadFloat4x4(reinterpret_cast<const XMFLOAT4X4*>(&view));

    XMMATRIX invProj = XMMatrixInverse(nullptr, xmProj);
    XMVECTOR viewNearH = XMVector4Transform(clipNear, invProj);
    XMVECTOR viewFarH = XMVector4Transform(clipFar, invProj);

    float wNear = XMVectorGetW(viewNearH);
    float wFar = XMVectorGetW(viewFarH);

    if (fabsf(wNear) < 1e-6f || fabsf(wFar) < 1e-6f) { return false; }

    viewNearH = XMVectorScale(viewNearH, 1.0f / wNear);
    viewFarH = XMVectorScale(viewFarH, 1.0f / wFar);

    XMMATRIX invView = XMMatrixInverse(nullptr, xmView);
    XMVECTOR worldNear = XMVector3TransformCoord(viewNearH, invView);
    XMVECTOR worldFar = XMVector3TransformCoord(viewFarH, invView);

    XMVECTOR dir = XMVector3Normalize(worldFar - worldNear);
    XMFLOAT3 df;
    XMStoreFloat3(&df, dir);

    outDir = Vector3(df.x, df.y, df.z);
    return true;
}


void ShootingComponent::Update(float dt)
{
    //経過時間更新
    m_timer += dt;

    //本体取得
    GameObject* owner = GetOwner();
    if (!owner) { return; }

    //色定義
    Vector4 colorC(1.0f, 1.0f, 0.0f, 1.0f);     //黄色
    Vector4 colorSpace(1.0f, 0.0f, 0.0f, 1.0f); //赤

	//追尾弾の発射（Cキー）
    bool cDown = Input::IsKeyDown('C');
    bool justPressedC = (cDown && !m_prevHomingKeyDown);
    m_prevHomingKeyDown = cDown;

    //Cキーが押されたら
    if (justPressedC)
    {

        if (m_timer < m_cooldown) { return; }
       
         //ターゲットを探す
         std::shared_ptr<GameObject> targetSp = FindBestHomingTarget();

         if (targetSp)
         {
             //ターゲットが見つかったらホーミング弾を撃つ
             FireHomingBullet(owner, targetSp);
             m_timer = 0.0f;
             return;
         }
         else
         { 
             //ターゲットがいない場合は発射しない
             return;
         }
    }

    //通常弾の発射（SPACEキー）
    bool wantFire = m_autoFire || Input::IsKeyDown(VK_SPACE);
    if (!wantFire) { return; }

    if (m_timer < m_cooldown) { return; }

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
        Matrix rotM = Matrix::CreateFromYawPitchRoll(rot.y, rot.x, rot.z);
        Vector3 spawnPos = owner->GetPosition() + Vector3::Transform(muzzleLocal, rotM);

        auto bullet = CreateBullet(spawnPos, forward, colorSpace);
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

    // カメラあり：レティクル方向に合わせて発射点・方向を決定（赤）
    Vector3 camPos = m_camera->GetPosition();
    Vector3 camDir = m_camera->GetAimDirectionFromReticle();
    if (camDir.LengthSquared() < 1e-6f)
    {
        camDir = m_camera->GetForward();
    }
    camDir.Normalize();

    Vector3 ownerForward = owner->GetForward();
    if (ownerForward.LengthSquared() < 1e-6f)
    {
        ownerForward = Vector3::UnitZ;
    }
    else
    {
        ownerForward.Normalize();
    }

    Vector3 muzzleGuess = owner->GetPosition() + ownerForward * m_spawnOffset;
    Vector3 camToMuzzle = muzzleGuess - camPos;
    float t = camToMuzzle.Dot(camDir);
    if (t < 1.0f)
    {
        t = 1.0f;
    }

    Vector3 spawnPos = camPos + camDir * t;
    Vector3 aimDir = camDir;

    auto bullet = CreateBullet(spawnPos, aimDir, colorSpace);
    if (bullet)
    {
        if (auto bc = bullet->GetComponent<BulletComponent>())
        {
            bc->SetVelocity(aimDir);
            bc->SetSpeed(m_bulletSpeed);
        }
        AddBulletToScene(bullet);
    }

    m_timer = 0.0f;
}

/// <summary>
/// 弾を実際に生成するための関数
/// </summary>
/// <param name="pos">弾の生成位置</param>
/// <param name="dir">弾の方向ベクトル</param>
/// <param name="color">弾の色</param>
/// <returns>生成した弾ベクトル</returns>
std::shared_ptr<GameObject> ShootingComponent::CreateBullet(const Vector3& pos, const Vector3& dir,
                                                            const Vector4& color)
{
    auto bullet = std::make_shared<Bullet>();
    bullet->SetPosition(pos);
    bullet->Initialize();

    auto bc = bullet->GetComponent<BulletComponent>();
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
        bc->SetBulletType(BulletComponent::PLAYER);
        bc->SetColor(color);
    }

    return bullet;
}

void ShootingComponent::AddBulletToScene(const std::shared_ptr<GameObject>& bullet)
{
    if (m_scene)
    {
        m_scene->AddObject(bullet);
        return;
    }

    if (auto owner = GetOwner())
    {
        if (owner->GetScene())
        {
            owner->GetScene()->AddObject(bullet);
        }
    }
}

