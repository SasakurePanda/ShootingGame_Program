#include "FixedTurretComponent.h"
#include "SceneManager.h"
#include "Bullet.h"
#include <iostream>

void FixedTurretComponent::Initialize()
{
    m_timer = 0.0f;
}

void FixedTurretComponent::Update(float dt)
{
    if (!GetOwner()) { return; }

    //ターゲットが存在しない場合はPlayerを取得
    if (auto sp = m_target.lock())
    {
        Vector3 myPos = GetOwner()->GetPosition();

		//ターゲットへのベクトル
        Vector3 toTarget = sp->GetPosition() - myPos;

        //Y軸だけ回転（水平面で向く）
        if (toTarget.LengthSquared() > 1e-6f)
        {
            toTarget.y = 0;
            toTarget.Normalize();
            float yaw = atan2(toTarget.x, toTarget.z);
            Vector3 rot = GetOwner()->GetRotation();
            rot.y = yaw;
            GetOwner()->SetRotation(rot);
        }

		
        // 射撃タイマー
        m_timer += dt;
        if (m_timer >= m_cooldown)
        {
            Shoot(toTarget);
            m_timer = 0.0f;
        }
    }
}

void FixedTurretComponent::Shoot(const Vector3& dir)
{
    auto owner = GetOwner();
    if (!owner) { return; }

    auto bullet = std::make_shared<Bullet>();
    bullet->SetPosition(owner->GetPosition() + Vector3(0, 3.0f, 0));

    auto bc = bullet->AddComponent<BulletComponent>();

    // 正しい方向ベクトルを正規化して渡す
    Vector3 nd = dir;
    if (nd.LengthSquared() > 1e-8f)
    {
        nd.Normalize();
    }
    else
    {
        nd = Vector3(0, 0, 1);      //フォールバック
    }

    bc->SetVelocity(nd);
    bc->SetSpeed(m_bulletSpeed);
    bc->SetBulletType(BulletComponent::ENEMY);
    
    bullet->Initialize();

    if (auto scene = owner->GetScene())
    {
        scene->AddObject(bullet);
    }
}
