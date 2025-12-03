#include "HitPointCompornent.h"
#include "GameObject.h"
#include <algorithm>

HitPointComponent::HitPointComponent(int maxHP)
	: m_maxHp(maxHP), m_hp(maxHP)
{
	//コンストラクタ
}

void HitPointComponent::Update(float dt)
{
	if(m_isInvincible)
	{
		m_invTimer -= dt;
		if (m_invTimer <= 0.0f)
		{
			m_isInvincible = false;
			m_invTimer = 0.0f;
		}
	}
}

bool HitPointComponent::ApplyDamage(const DamageInfo& info)
{
	//既に死んでいるなら
	if (m_hp <= 0){ return false; }

	//無敵状態ならダメージを無視
	if (m_isInvincible && !info.ignoreInvincibility){ return false; }

	m_hp -= info.amount; //ダメージ分HPを減らす
	if (m_hp < 0) { m_hp = 0; };
	if (m_onDamaged) { m_onDamaged(info); };

	if(m_invOnHit > 0.0f)
	{
		m_isInvincible = true;
		m_invTimer = m_invOnHit;
	}

	if (m_hp == 0)
	{
		if (m_onDeath) { m_onDeath(); }
	}

	return true;
}

void HitPointComponent::Heal(int amount)
{
	if (amount <= 0) { return; }
	m_hp += amount;
	if (m_onHealed) { m_onHealed(amount); }
	if (m_hp > m_maxHp) { m_hp = m_maxHp; }
}

void HitPointComponent::SetInvincible(float seconds)
{
	if (seconds > 0.0f)
	{
		m_isInvincible = true;
		m_invTimer = seconds;
	}
	else
	{
		m_isInvincible = false;
		m_invTimer = 0.0f;
	}
}

