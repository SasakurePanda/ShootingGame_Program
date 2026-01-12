#pragma once
#include <string>
#include "Component.h"

class DamageComponent : public Component
{
public:
	DamageComponent();
	~DamageComponent();

	void Initialize() override {};

	void Update(float dt) override {};

	//----------------------SetŠÖ”ŠÖ˜A------------------------
	void SetDamage(int d) { m_damage = d; }
	//void SetTag(const std::string& tag) { m_tag = tag; }

	//----------------------GetŠÖ”ŠÖ˜A------------------------
	int GetDamage() const { return m_damage; }
	//std::string GetTag() const { return m_tag; }

private:
	int m_damage = 1;
	//std::string m_tag;
};