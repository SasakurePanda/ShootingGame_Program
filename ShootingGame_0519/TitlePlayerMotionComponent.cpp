#include "TitlePlayerMotionComponent.h"
#include "GameObject.h"
#include <iostream>

void TitlePlayerMotionComponent::Initialize()
{

}

void TitlePlayerMotionComponent::Update(float dt)
{
    if (dt <= 0.0f) { return; }

    GameObject* owner = GetOwner();
    if (!owner) { return; }

    m_Time += dt;

    float time = m_Time / max(0.01f, m_Duration);
    float t = std::clamp(time, 0.0f, 1.0f);

    DirectX::SimpleMath::Vector3 pos = EvaluateBezier(t);
    owner->SetPosition(pos);

    // 進行方向
    float tNext = std::clamp(t + (1.0f / max(1.0f, m_Duration * 60.0f)), 0.0f, 1.0f);
    DirectX::SimpleMath::Vector3 posNext = EvaluateBezier(tNext);
    DirectX::SimpleMath::Vector3 dir = posNext - pos;

    if (dir.LengthSquared() <= 1e-6f)
    {
        return;
    }
    dir.Normalize();

    float yaw = std::atan2(dir.x, dir.z);
    float pitch = -std::atan2(dir.y, std::sqrt(dir.x * dir.x + dir.z * dir.z));

    // まずこれを試す（前後反転）
    yaw -= DirectX::XM_PIDIV2;

    // スムーズ回転
    float lerpRate = 8.0f;
    float a = std::min(1.0f, lerpRate * dt);

    DirectX::SimpleMath::Vector3 curRot = owner->GetRotation();
    DirectX::SimpleMath::Vector3 targetRot(pitch/* + -0.9f*/, yaw, 0.0f);
    DirectX::SimpleMath::Vector3 newRot = curRot + (targetRot - curRot) * a;

    owner->SetRotation(newRot);

    if (!m_HasTriggered && t >= m_TriggerT)
    {
        m_HasTriggered = true;
        if (m_OnLogoTrigger)
        {
            m_OnLogoTrigger();
        }
    }
}


void TitlePlayerMotionComponent::SetControlPoints(const DirectX::SimpleMath::Vector3& p0,
												  const DirectX::SimpleMath::Vector3& p1,
												  const DirectX::SimpleMath::Vector3& p2,
												  const DirectX::SimpleMath::Vector3& p3)
{
	m_p01 = p0;
	m_p02 = p1;
	m_p03 = p2;
	m_p04 = p3;
}

/// <summary>
/// 指定した数値で3次ベジェ曲線を評価する
/// </summary>
/// <param name="t"> ベジェ曲線の進み具合 </param>
/// <returns>  曲線上の今の位置 </returns>
DirectX::SimpleMath::Vector3 TitlePlayerMotionComponent::EvaluateBezier(float t) const
{
	t = std::clamp(t, 0.0f, 1.0f);

	//残りの割合・二乗・三乗を先に計算しておく
	float u = 1.0f - t;
	float uu = u * u;
	float tt = t * t;
	float uuu = uu * u;
	float ttt = tt * t;

	DirectX::SimpleMath::Vector3 p = (m_p01 * uuu) +
								     (m_p02 * (3.0f * uu * t)) +
								     (m_p03 * (3.0f * u * tt)) +
									 (m_p04 * ttt);
	return p;
}