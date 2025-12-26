#include <iostream>
#include "Enemy.h"
#include "Bullet.h"
#include "HitPointCompornent.h"

void Enemy::Initialize()
{
    GameObject::Initialize();
}

void Enemy::Update(float dt)
{
    auto hp = GetComponent<HitPointComponent>();
    //std::cout << "敵のHP：" << hp->GetHP() << std::endl;

    //コンポーネント等のUpdate
    GameObject::Update(dt);
}

//衝突時の処理
void Enemy::OnCollision(GameObject* other)
{
    if (!other)
    {
        return;
    }

    //相手が弾かどうか
    if (auto bulletComp = other->GetComponent<BulletComponent>()) 
    {
        if (bulletComp->GetBulletType() == BulletComponent::BulletType::PLAYER)
        { 
            auto hp = GetComponent<HitPointComponent>();
            
            DamageInfo di;
            di.amount = 1;
            di.instigator = other;
            di.tag = "player_bullet";
            bool applied = hp->ApplyDamage(di);

            if (hp->GetHP() <= 0)
            {
                if (auto scene = GetScene())
                {
                    scene->RemoveObject(this); // GameScene::RemoveObject(GameObject*) があるはず
                    scene->RemoveObject(other);
                }
            }
        }
    }
}

void Enemy::Damage(int amount)
{
    if (amount <= 0) return;
    m_hp -= amount;

    if (m_hp <= 0)
    {
        // 呼び出し順：派生の OnDeath をまず呼ばせる（派生が特殊演出を行えるように）
        OnDeath();
        // その後共通ハンドラ
        HandleDeathCommon();
    }
}

void Enemy::Heal(int amount)
{
    if (amount <= 0)
    {
        return;
    }
    m_hp += amount;
}

void Enemy::OnDeath()
{
    // デフォルトは何もしない（派生でエフェクトや音、スコア加算を行う）
    // 例（参考）: std::cout << "Enemy died\n";
}

void Enemy::HandleDeathCommon()
{
    // 共通の削除処理など（スコア付与やパーティクル生成の共通処理をここに書ける）
    // ここでは単純にシーンから削除
    RemoveSelfFromScene();
}

void Enemy::RemoveSelfFromScene()
{
    IScene* s = GetScene();
    if (s)
    {
        s->RemoveObject(this);
    }
    else
    {
        // シーンがない場合はログだけ（安全のため）
        // std::cout << "Warning: Enemy removed but no scene attached.\n";
    }
}

//～dynamic_castとは～
//基底クラスと派生クラスのポインタ/参照の変換
//実行時に型のチェックをし、失敗したときは nullptr(ポインタの場合)になる
//ポリモーフィズムが有効な型(virtual 必須)でしか使えない
//上記の場合だとGameObject*をBullet*に変換しています
//(今は使ってないです)