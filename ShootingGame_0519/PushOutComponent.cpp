#include "PushOutComponent.h"
#include "GameObject.h"

using namespace DirectX::SimpleMath;

void PushOutComponent::AddPush(const DirectX::SimpleMath::Vector3& push)
{
	m_accumulatedPush += push;
	m_isPush = true;
}

void PushOutComponent::ApplyPush()
{
	if (m_accumulatedPush.LengthSquared() > 1e-6f)
	{
		GetOwner()->SetPosition(GetOwner()->GetPosition() + m_accumulatedPush);
	}
	m_accumulatedPush = Vector3::Zero;
}
