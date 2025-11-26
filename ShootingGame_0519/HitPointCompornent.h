#pragma once
#include "Component.h"
#include <functional>
#include <string>

class GameObject;

//ダメージ処理で使う用のインフォメーション
struct DamageInfo
{
	int amount = 1;                   //ダメージ量
    GameObject* instigator = nullptr; //ダメージ源
    std::string tag;                  //
    bool ignoreInvincibility = false; // 例外処理用
    //将来的に knockback などを追加可能
};

class HitPointComponent : public Component
{
public:
    HitPointComponent(int maxHP);

    void Initialize() override {}
    void Update(float dt) override;

    bool ApplyDamage(const DamageInfo& info); //Damageを入れる
    void Heal(int amount);                    //HPを回復する

    //設定/取得
    int GetHP() const { return m_hp; }
    int GetMaxHP() const { return m_maxHp; }
    bool IsDead() const { return m_hp <= 0; }

    //無敵(被弾後自動で入れるなら m_invOnHit を使う)
    void SetInvincibilityOnHit(float seconds) { m_invOnHit = seconds; }
    void SetInvincible(float seconds);

    void SetOnDamaged(std::function<void(const DamageInfo&)> cb) { m_onDamaged = std::move(cb); }
    void SetOnHealed(std::function<void(int)> cb) { m_onHealed = std::move(cb); }
    void SetOnDeath(std::function<void()> cb) { m_onDeath = std::move(cb); }

private:
	int m_hp;       // 現在のHP
	int m_maxHp;    // 最大HP
	bool m_isInvincible = false;    //無敵状態かのフラグ
	float m_invTimer = 0.0f;        //無敵時間用のタイマー
	float m_invOnHit = 1.5f;        //被弾後に自動で無敵になる時間

	std::function<void(const DamageInfo&)> m_onDamaged;     //ダメージ時コールバック
	std::function<void(int)> m_onHealed;                    //回復時コールバック
	std::function<void()> m_onDeath;                        //死亡時コールバック
};