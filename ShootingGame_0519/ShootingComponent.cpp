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
    if (!cam) { return false; }

    float width = Application::GetWidth();
    float height = Application::GetHeight();
    if (width <= 0.0f || height <= 0.0f) return false;

    // View / Proj
    SMS::Matrix view = cam->GetView();
    SMS::Matrix proj = cam->GetProj();

    // Pixel center correction
    float fx = static_cast<float>(mousePos.x) + 0.5f;
    float fy = static_cast<float>(mousePos.y) + 0.5f;

    // NDC (DirectX: x [-1,1], y [-1,1], z [0,1])
    float xN = (2.0f * fx / width) - 1.0f;
    float yN = 1.0f - (2.0f * fy / height);

    XMVECTOR clipNear = XMVectorSet(xN, yN, 0.0f, 1.0f);
    XMVECTOR clipFar  = XMVectorSet(xN, yN, 1.0f, 1.0f);

    XMMATRIX xmProj = XMLoadFloat4x4(reinterpret_cast<const XMFLOAT4X4*>(&proj));
    XMMATRIX xmView = XMLoadFloat4x4(reinterpret_cast<const XMFLOAT4X4*>(&view));

    // invProj: clip -> view
    XMMATRIX invProj   = XMMatrixInverse(nullptr, xmProj);
    XMVECTOR viewNearH = XMVector4Transform(clipNear, invProj);
    XMVECTOR viewFarH  = XMVector4Transform(clipFar, invProj);

    float wNear = XMVectorGetW(viewNearH);
    float wFar  = XMVectorGetW(viewFarH);
    if (fabsf(wNear) < 1e-6f || fabsf(wFar) < 1e-6f) { return false; }

    viewNearH = XMVectorScale(viewNearH, 1.0f / wNear);
    viewFarH = XMVectorScale(viewFarH, 1.0f / wFar);

    XMMATRIX invView = XMMatrixInverse(nullptr, xmView);
    XMVECTOR worldNear = XMVector3TransformCoord(viewNearH, invView);
    XMVECTOR worldFar = XMVector3TransformCoord(viewFarH, invView);

    XMFLOAT3 nearF3, farF3;
    XMStoreFloat3(&nearF3, worldNear);
    XMStoreFloat3(&farF3, worldFar);

    outNear = SMS::Vector3(nearF3.x, nearF3.y, nearF3.z);
    outFar = SMS::Vector3(farF3.x, farF3.y, farF3.z);

    return true;
}


void ShootingComponent::Update(float dt)
{
    //タイマーを更新する
    m_timer += dt;

    //もとのオブジェクトがいるかどうかをチェック
    GameObject* owner = GetOwner();
    if (!owner) { return; }

    //発射の入力が出ているかどうかを確認
    bool wantFire = Input::IsKeyDown(VK_SPACE);
    if (!wantFire) { return; }

    //クールダウン管理
    if (m_timer < m_cooldown) { return; }

    Vector3 ownerFwd = owner->GetForward();
    //前向きベクトルがある程度の長さがあるなら
    if (ownerFwd.LengthSquared() > 1e-6f)
    {
        //正規化
        ownerFwd.Normalize();
    }
    else
    {
        //短いなら別で用意(Z軸にオフセット)
        ownerFwd = Vector3(0, 0, 1);
    }

    //発射位置をPlayerのポジションから前に少しずらす
	Vector3 spnwPos = owner->GetPosition() + ownerFwd * m_spawnOffset;

    //マウスの座標を取得
    POINT reticlePx = Input::GetMousePosition();

	//ニアファー点の変数
    Vector3 nearPt, farPt;
    bool gotRay = false;

    if (m_camera)
    {
        gotRay = GetMousePickRay(m_camera, reticlePx, nearPt, farPt);
    }

    Vector3 rayDir;
    if (gotRay)
    {
		//ニア点からファー点へのベクトルを計算
        rayDir = farPt - nearPt;
        float rl = rayDir.Length();
        if (rl > 1e-6f) 
        {
            rayDir /= rl;
        }
        else
        {
            gotRay = false;
        }
    }

    if (!gotRay)
    {
        if (m_camera)
        {
            rayDir = m_camera->GetForward();
            if (rayDir.LengthSquared() > 1e-6f)
            {
                rayDir.Normalize();
            }
        }
        if (rayDir.LengthSquared() < 1e-6f)
        {
            rayDir = ownerFwd; // 既に正規化済み
        }
    }
    
    Vector3 aimPoint;
    bool usedHit = false;
    const float aimDist = m_aimDistance;

    if (m_scene && gotRay)
    {
        RaycastHit hit;

        Vector3 origin = nearPt;

        //今いるシーンにアクセスしてRaycast関数を呼ぶ
        if (m_scene->Raycast(origin, rayDir, aimDist, hit, owner))
        {
            //当たったものがEnemyかを確認
            if (hit.hitObject && dynamic_cast<Enemy*>(hit.hitObject.get()))
            {
                aimPoint = hit.position;
                usedHit = true;
            }
        }
    }

    if (!usedHit)
    {
        Vector3 camOrigin;
        if (m_camera) 
        {
            camOrigin = m_camera->GetPosition();
        }
        else 
        {
            camOrigin = nearPt;
        }
        aimPoint = camOrigin + rayDir * aimDist;
    }

    //発射方向を spawnPos からのベクトルで決める
    Vector3 aimDir = aimPoint - spnwPos;
    float dlen = aimDir.Length();
    if (dlen > 1e-6f)
    {
        aimDir /= dlen;
    }
    else 
    {   
        //万が一 spawnPos とほぼ同じなら rayDir を使う
        aimDir = rayDir;
    } 

    //弾を生成して方向と速度を渡す
    auto bullet = CreateBullet(spnwPos, aimDir);
    if (bullet)
    {
        if (auto bc = bullet->GetComponent<BulletComponent>())
        {
            //API による：SetVelocity が「方向（normalized）」を期待するならこの形
            bc->SetVelocity(aimDir);
            bc->SetSpeed(m_bulletSpeed);

            // 必要ならホーミング等を無効化
            // bc->SetHomingStrength(0.0f);
        }

        if (m_scene)
        {
        
            m_scene->AddObject(bullet);
        }
        else if (owner->GetScene())
        {
            owner->GetScene()->AddObject(bullet);
        }
    }

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



