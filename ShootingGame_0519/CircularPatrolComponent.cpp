#include "CircularPatrolComponent.h"
#include "GameObject.h"
#include <cmath>

using namespace DirectX::SimpleMath;

void CirculPatrolComponent::Initialize()
{
    //半径がより小さいなら
    if (m_Radius <= 0.0f)
    {
        return;
    }

    Vector3 pos = GetOwner()->GetPosition();
    Vector3 rel = pos - m_Center;
    // XZ 平面の角度
    if (rel.LengthSquared() > 1e-6f)
    {
        m_Angle = std::atan2(rel.x, rel.z); // atan2(x,z) の慣習に合わせる
    }
    else
    {
        // 既定の角度 (0) の位置に配置する
        Vector3 newPos = m_Center + Vector3(std::sin(m_Angle) * m_Radius, 0.0f, std::cos(m_Angle) * m_Radius);
        GetOwner()->SetPosition(newPos);
    }

}

void CirculPatrolComponent::Update(float dt)
{
    //親オブジェクトがないなら
    if (!GetOwner()) { return; }
    //向かう地点が何もない場合
    if (m_Radius <= 0.0f) { return; }
    //デルタタイムが0より小さい場合
    if (dt <= 0.0f) { return; }

    float dir;
    if (m_Clockwise)
    {
        dir = -1.0f;
    }
    else
    {
        dir = 1.0f;
    }

    m_Angle += dir * m_AngularSpeed * dt;

    // 位置更新（XZ平面）
    Vector3 newPos = m_Center + Vector3(std::sin(m_Angle) * m_Radius, 0.0f, std::cos(m_Angle) * m_Radius);
    GetOwner()->SetPosition(newPos);

    // 向きを接線方向に合わせる（簡易）
    if (m_RotateToTangent)
    {
        // 接線ベクトル（d/dθ）
        Vector3 tangent = Vector3(std::cos(m_Angle) * dir, 0.0f, -std::sin(m_Angle) * dir);
        if (tangent.LengthSquared() > 1e-6f) tangent.Normalize();

        Vector3 rot = GetOwner()->GetRotation();
        float yaw = std::atan2(tangent.x, tangent.z);
        rot.y = yaw;
        GetOwner()->SetRotation(rot);
    }
}