#pragma once
#include "Component.h"
#include "IScene.h"
#include "GameObject.h"
#include "BulletComponent.h"
#include <SimpleMath.h>
#include <memory>

using namespace DirectX::SimpleMath;

class FixedTurretComponent : public Component
{
public:
    FixedTurretComponent() = default;
    ~FixedTurretComponent() override = default;

    void Initialize() override
    {
        m_timer = 0.0f;
    }

    void Update(float dt) override
    {
        if (!GetOwner()) return;

        // ターゲットが存在しない場合はPlayerを取得
        if (auto sp = m_target.lock())
        {
            Vector3 myPos = GetOwner()->GetPosition();
            Vector3 toTarget = sp->GetPosition() - myPos;

            // Y軸だけ回転（水平面で向く）
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

    void SetTarget(std::weak_ptr<GameObject> t) { m_target = t; }
    void SetCooldown(float cd) { m_cooldown = cd; }
    void SetBulletSpeed(float sp) { m_bulletSpeed = sp; }

private:
    std::weak_ptr<GameObject> m_target;
    float m_cooldown = 1.0f;   // 発射間隔
    float m_timer = 0.0f;
    float m_bulletSpeed = 50.0f;

    void Shoot(const Vector3& dir)
    {
        auto owner = GetOwner();
        if (!owner) return;

        auto bullet = std::make_shared<GameObject>();
        bullet->SetPosition(owner->GetPosition() + Vector3(0, 1.0f, 0)); // 少し上から発射
        auto bc = std::make_shared<BulletComponent>();
        bc->SetVelocity(dir);
        bc->SetSpeed(m_bulletSpeed);
        bc->SetBulletType(BulletComponent::ENEMY);
        bullet->AddComponent(bc);
        bullet->Initialize();

        if (auto scene = owner->GetScene())
        {
            scene->AddObject(bullet);
        }
    }
};
