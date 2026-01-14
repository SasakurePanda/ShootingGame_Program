#include "PushOutComponent.h"
#include "GameObject.h"

using namespace DirectX::SimpleMath;

void PushOutComponent::Initialize()
{
	//‰Šú‰»
	m_isPush = false;
	m_pushVector = Vector3::Zero;
}

void PushOutComponent::Update(float dt)
{
	if(!m_isPush){ return; }

	GameObject* owner = GetOwner();
	if (!owner) { return; }

	Vector3 pos = owner->GetPosition();
	pos += m_pushVector;
	owner->SetPosition(pos);

	//ƒŠƒZƒbƒg
	m_isPush = false;
	m_pushVector = Vector3::Zero;
}

void PushOutComponent::AddPush(const DirectX::SimpleMath::Vector3& push)
{
	if (push.LengthSquared() < 1e-8f){ return; }

	m_pushVector += push;
	m_isPush = true;
}

void PushOutComponent::ClearPush()
{
	m_pushVector = Vector3::Zero;
	m_isPush = false;
}