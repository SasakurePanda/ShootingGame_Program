#include "MoveComponent.h"
#include "GameObject.h"
#include "Application.h"
#include <iostream>
//#include <DirectXMath.h>

using namespace DirectX::SimpleMath;

constexpr float XM_PI = 3.14159265f;
constexpr float XM_2PI = 6.28318530f;

void MoveComponent::Initialize()
{


}

void MoveComponent::Update(float dt)
{
    if (!m_camera) return;

    // 1) 目標点（ワールド）をカメラから取得
    Vector3 aimTarget = m_camera->GetAimPoint();

    // 2) 現在の位置と向き
    Vector3 pos = GetOwner()->GetPosition();
    Vector3 rot = GetOwner()->GetRotation(); // rot.x=pitch, rot.y=yaw, rot.z=roll の想定
    float currentYaw = rot.y;

    // 3) 目標方向（水平のみ）
    Vector3 toTarget = aimTarget - pos;
    toTarget.y = 0.0f;
    if (toTarget.LengthSquared() < 1e-6f)
    {
        Vector3 forward = Vector3(std::sin(currentYaw), 0.0f, std::cos(currentYaw));
        forward.Normalize();
        pos += forward * m_speed * dt;
        GetOwner()->SetPosition(pos);
        return;
    }
    toTarget.Normalize();

    // 4) 目標 yaw を求める（forward 定義と整合）
    float targetYaw = std::atan2(toTarget.x, toTarget.z);

    // 5) yaw 差のラップ
    float delta = targetYaw - currentYaw;
    while (delta > XM_PI) delta -= XM_2PI;
    while (delta < -XM_PI) delta += XM_2PI;

    // 6) 角速度上限で回す
    float maxTurn = m_rotateSpeed * dt;
    float applied = std::clamp(delta, -maxTurn, maxTurn);
    currentYaw += applied;

    // 7) ロール（バンク）演出（lerp で滑らかに）
    Vector3 currentForward = Vector3(std::sin(currentYaw), 0.0f, std::cos(currentYaw));
    currentForward.Normalize();
    float lateral = currentForward.x * toTarget.z - currentForward.z * toTarget.x; // cross.y
    constexpr float DegToRad = 3.14159265358979323846f / 180.0f;
    float maxBankAngle = 20.0f * DegToRad; // 20 deg
    float desiredRoll = -lateral * maxBankAngle;
    float bankSmooth = 6.0f; // 値を変えてフィールを調整
    m_currentRoll = m_currentRoll + (desiredRoll - m_currentRoll) * std::min(1.0f, bankSmooth * dt);

    // 8) 回転と位置を反映
    rot.y = currentYaw;
    rot.z = m_currentRoll;
    GetOwner()->SetRotation(rot);

    // 9) 常に前進
    pos += currentForward * m_speed * dt;
    GetOwner()->SetPosition(pos);
}
