#include "BulletComponent.h"

void BulletComponent::Initialize()
{
    if (auto owner = GetOwner())
    {
        // プレイヤーの Y軸回転を使って「前方ベクトル」を作成
        float yaw = owner->GetRotation().y;

        // Y軸回転の行列から前方向を取得
        DirectX::SimpleMath::Matrix rot = DirectX::SimpleMath::Matrix::CreateRotationY(yaw);
        Vector3 forward = rot.Forward();

         m_velocity = forward;
    }
    else
    {
        m_velocity = Vector3::Forward; // フォールバック（万が一）
    }
    m_speed = 20.0f;
}

void BulletComponent::Update(float dt)
{
    if (auto owner = GetOwner())
    {
        Vector3 pos = owner->GetPosition();
        pos += m_velocity * m_speed * 0.016f; // 固定60FPSならこのまま
        owner->SetPosition(pos);
    }
}