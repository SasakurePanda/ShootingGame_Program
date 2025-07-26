#include "ShootingComponent.h"
#include <Windows.h>

void ShootingComponent::Update()
{
    float deltaTime = 1.0f / 60.0f; // 仮に固定60FPS前提（本来は渡すべき）

    m_timer += deltaTime;

    if (Input::IsKeyDown(VK_SPACE) && m_timer >= m_cooldown)
    {
        m_timer = 0.0f; // タイマーリセット

        auto owner = GetOwner();
        if (!owner) return;

        auto bullet = std::make_shared<Bullet>();
        bullet->SetPosition(owner->GetPosition());
        bullet->Initialize();

        if (m_scene) m_scene->AddObject(bullet);
    }
}