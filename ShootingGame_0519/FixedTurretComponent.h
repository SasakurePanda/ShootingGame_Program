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

    void Initialize() override;

    void Update(float dt) override;

    void SetTarget(std::weak_ptr<GameObject> t) { m_target = t; }
    void SetCooldown(float cd) { m_cooldown = cd; }
    void SetBulletSpeed(float sp) { m_bulletSpeed = sp; }

private:
    std::weak_ptr<GameObject> m_target;
    float m_cooldown = 1.0f;   // ”­ŽËŠÔŠu
    float m_timer = 0.0f;
    float m_bulletSpeed = 50.0f;

    void Shoot(const Vector3& dir);
};
