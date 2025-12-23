#include "PatrolComponent.h"
#include "GameObject.h"
#include <iostream>
#include <cmath>

using namespace DirectX::SimpleMath;

void PatrolComponent::Initialize()
{
	//向かう地点が最低一つは設定されており、かつ今設定されているインデックスが
	//全ウェイポイント数以上の場合
	if (!m_waypoints.empty() && m_currentIndex >= m_waypoints.size())
	{
		m_currentIndex = 0;
	}

	//初回の向きを整える
	if (!m_waypoints.empty() && GetOwner())
	{
		Vector3 pos = GetOwner()->GetPosition();
		Vector3 toTarget = m_waypoints[m_currentIndex] - pos;

		toTarget.y = 0.0f;
		if (toTarget.LengthSquared() > 1e-6f)
		{

			toTarget.Normalize();
			m_currentDir = toTarget;
		}

	}
}

void PatrolComponent::Update(float dt)
{
	//親オブジェクトがないなら
	if (!GetOwner()){return;}
	//向かう地点が何もない場合
	if (m_waypoints.empty()) { return; }
	//デルタタイムが0より小さい場合
	if (dt <= 0.0f) { return; }

	GameObject* owner = GetOwner();

	Vector3 pos = owner->GetPosition();
	Vector3 Target = m_waypoints[m_currentIndex];

	Vector3 toTarget = Target - pos;

	//toTargetの長さを取得
	float dist = toTarget.Length();

	//到着判定を取る
	if(dist <= m_arrivalThreshold)
	{
		if (m_onReached)
		{
			m_onReached(m_currentIndex);
		}

		AdvanceWaypoint();
		return;
	}

	//目的方向
	Vector3 desiredDir = toTarget;
	desiredDir.y = 0.0f;

	if (desiredDir.LengthSquared() <= 1e-6f)
	{
		return;
	}

	desiredDir.Normalize();

	//進行方向を補完して曲線に
	float t = m_turnRate * dt;
	t = std::clamp(t, 0.0f, 1.0f);

	m_currentDir = Vector3::Lerp(m_currentDir, desiredDir, t);

	if(m_currentDir.LengthSquared() > 1e-6f)
	{
		m_currentDir.Normalize();
	}
	else
	{
		m_currentDir = desiredDir;
	}

	//近づくほど減速してみる
	float speedScale = 1.0f;
	if (m_slowRadius > m_arrivalThreshold)
	{
		speedScale = dist / m_arrivalThreshold;
		speedScale = std::clamp(speedScale, 0.3f, 1.0f);
	}

	// 移動
	pos += m_currentDir * (m_speed * speedScale) * dt;
	owner->SetPosition(pos);

	// 向き更新
	if (m_faceMovement)
	{
		Vector3 rot = owner->GetRotation();
		float yaw = std::atan2(m_currentDir.x, m_currentDir.z);
		rot.y = yaw;
		owner->SetRotation(rot);
	}
}

void PatrolComponent::SetWaypoints(const std::vector<Vector3>& pts)
{
	m_waypoints = pts;
	if (m_currentIndex >= m_waypoints.size())
	{
		m_currentIndex = 0;
	}
}

void PatrolComponent::SetWaypoints(std::vector<Vector3>&& pts)
{
	m_waypoints = std::move(pts);
	if (m_currentIndex >= m_waypoints.size())
	{
		m_currentIndex = 0;
	}
}

void PatrolComponent::Reset()
{
	m_currentIndex = 0;
	m_dir = 1;
	m_currentDir = Vector3(0.0f, 0.0f, 1.0f);
}

void PatrolComponent::AdvanceWaypoint()
{
	if (m_pingPong)
	{
		int next = static_cast<int>(m_currentIndex) + m_dir;

		if (next < 0 || next >= static_cast<int>(m_waypoints.size()))
		{
			m_dir = -m_dir;
			next = static_cast<int>(m_currentIndex) + m_dir;

			if (next < 0)
			{
				next = 0;
			}
			if (next >= static_cast<int>(m_waypoints.size()))
			{
				next = static_cast<int>(m_waypoints.size() - 1);
			}
		}

		m_currentIndex = static_cast<size_t>(next);
	}
	else
	{
		if (m_currentIndex + 1 < m_waypoints.size())
		{
			m_currentIndex++;
		}
		else
		{
			if (m_loop)
			{
				m_currentIndex = 0;
			}
			else
			{
				// 停止したい場合はここで return したり有効フラグを折る
			}
		}
	}
}

