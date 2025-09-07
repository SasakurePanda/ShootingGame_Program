#pragma once
#include "Component.h"

class BulletComponent : public Component
{
public:
    BulletComponent() = default;

    void Initialize() override;

    void Update(float dt) override;

    void SetVelocity(const Vector3& dir) { m_velocity = dir; }
    void SetSpeed(float speed) { m_speed = speed; }

private:
    Vector3 m_velocity;
    float m_speed;
};

