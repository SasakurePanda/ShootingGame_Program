#include "ShootingComponent.h"
#include <Windows.h>

void ShootingComponent::Update(float dt)
{
    float deltaTime = 1.0f / 60.0f;
    m_timer += deltaTime;

    if (Input::IsKeyDown(VK_SPACE) && m_timer >= m_cooldown)
    {
        m_timer = 0.0f;

        auto owner = GetOwner();
        if (!owner || !m_camera) return;

        //メラの向いてる方向を使う！
        Vector3 forward = m_camera->GetForward();
        forward.Normalize();

        // 弾生成
        auto bullet = std::make_shared<Bullet>();
        bullet->SetPosition(owner->GetPosition() + forward * 1.5f); // ちょっと前から出す
        bullet->Initialize();

        auto bulletComp = bullet->GetComponent<BulletComponent>();
        if (bulletComp)
        {
            bulletComp->SetVelocity(forward);
            bulletComp->SetSpeed(20.0f);
        }

        if (m_scene) m_scene->AddObject(bullet);
    }
}
