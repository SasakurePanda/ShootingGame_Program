#include "ShootingComponent.h"
#include "Bullet.h"
#include "Input.h"
#include "GameObject.h"
#include "IScene.h"
#include "Application.h"

#include <iostream>
#include <DirectXMath.h>
using namespace DirectX;
using namespace DirectX::SimpleMath;

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

    // 発射入力（SPACE 押しっぱなしで連射）
    bool wantFire = m_autoFire || Input::IsKeyDown(VK_SPACE);
    if (!wantFire) { return; }

    // クールタイム未経過なら撃たない
    if (m_timer < m_cooldown) { return; }

    // ========= プレイヤー前方向 =========
    Vector3 forward = owner->GetForward();
    if (forward.LengthSquared() < 1e-6f)
    {
        forward = Vector3::UnitZ;
    }
    else
    {
        forward.Normalize();
    }

    // ========= 発射位置（マズル） =========
    Vector3 muzzleLocal(0.0f, 0.0f, m_spawnOffset);

    // Player の回転（YawPitchRoll ベース）
    Vector3 rot = owner->GetRotation();
    Matrix rotM = Matrix::CreateFromYawPitchRoll(rot.y, rot.x, rot.z);

    // ワールド位置
    Vector3 spawnPos = owner->GetPosition() + Vector3::Transform(muzzleLocal, rotM);

    // ========= AimPoint から弾の向きを決める =========
    Vector3 aimDir = forward; // デフォルトは前方

    if (m_camera)
    {
        // カメラが計算している「レティクルの先の 3D 位置」
        Vector3 aimPoint = m_camera->GetAimPoint();

        // 銃口 → AimPoint 方向
        Vector3 toAim = aimPoint - spawnPos;
        if (toAim.LengthSquared() > 1e-6f)
        {
            toAim.Normalize();
            aimDir = toAim;
        }
    }

    // ========= 弾生成 =========
    auto bullet = CreateBullet(spawnPos, aimDir);
    if (bullet)
    {
        if (auto bc = bullet->GetComponent<BulletComponent>())
        {
            bc->SetVelocity(aimDir);
            bc->SetSpeed(m_bulletSpeed);
        }

        AddBulletToScene(bullet);
    }

    // クールタイムリセット
    m_timer = 0.0f;
}


std::shared_ptr<GameObject> ShootingComponent::CreateBullet(const Vector3& pos, const Vector3& dir)
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

		//std::cout << "弾の方向: (" << d.x << ", " << d.y << ", " << d.z << ")\n";

        bc->SetVelocity(d);          // 方向（正規化）
        bc->SetSpeed(m_bulletSpeed); // 速さ
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



