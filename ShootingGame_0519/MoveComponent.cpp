#include "MoveComponent.h"
#include "GameObject.h"

using namespace DirectX::SimpleMath;

void MoveComponent::Initialize()
{

}

void MoveComponent::Update()
{
    if (!m_camera) return;

    Vector3 forward = m_camera->GetForward();
    Vector3 right = m_camera->GetRight();

    // 上下成分を無視して地面に平行に移動
    forward.y = 0.0f;
    right.y = 0.0f;
    forward.Normalize();
    right.Normalize();

    Vector3 move = Vector3::Zero;

    if (Input::IsKeyDown('W')) move += forward;
    if (Input::IsKeyDown('S')) move -= forward;
    if (Input::IsKeyDown('D')) move += right;
    if (Input::IsKeyDown('A')) move -= right;

    if (move.LengthSquared() > 0.0f)
    {
        move.Normalize();
        move *= m_speed * (1.0f / 60.0f); // 固定フレーム用

        Vector3 pos = GetOwner()->GetPosition();
        pos += move;
        GetOwner()->SetPosition(pos);

        // 進行方向に回転（オプション）
        float angleY = std::atan2(move.x, move.z); // Y軸回転
        Vector3 rot = GetOwner()->GetRotation();
        rot.y = angleY;
        GetOwner()->SetRotation(rot);
    }
}
