#pragma once
#include "Component.h"
#include <SimpleMath.h>

class PushOutComponent : public Component
{
public:
	PushOutComponent()  {};
	~PushOutComponent() {};

	void Initialize() override;
	void Update(float dt) override;

	//-------------Set関数--------------
	void SetMass(float m) { m_mass = m; }

	//-------------Get関数--------------
	float GetMass() const { return m_mass; }

	//-----------その他関数-------------
	void AddPush(const DirectX::SimpleMath::Vector3& push);
	void ClearPush();

private:
	DirectX::SimpleMath::Vector3 m_pushVector = DirectX::SimpleMath::Vector3::Zero; //押し出しベクトル
	float m_mass = 1.0f;   //押し出しの重み
	bool m_isPush = false; //押し出しがあるかどうか
};