#include "Player.h"
#include "Building.h"
#include "ModelComponent.h"
#include "MoveComponent.h"
#include "ShootingComponent.h"
#include "AABBColliderComponent.h"
#include "OBBColliderComponent.h"
#include "HitPointCompornent.h"
#include "BulletComponent.h"
#include "Collision.h" 
#include <iostream>

void Player::Initialize()
{
    //基底クラスの初期化（Component群）
    GameObject::Initialize(); 

    //モデルコンポーネントの生成
    auto modelComp = std::make_shared<ModelComponent>();
    //モデルの読み込み
    modelComp->LoadModel("Asset/Model/Robot/12211_Robot_l2.obj");
    modelComp->SetColor(Color(1, 0, 0, 1));

    //移動コンポーネントの生成
    auto moveComp = std::make_shared<MoveComponent>();
    //弾発射コンポーネントの生成
    auto shootComp = std::make_shared<ShootingComponent>();
    //HPコンポーネントの生成
    auto HPComp = std::make_shared<HitPointComponent>(20);
    HPComp->SetInvincibilityOnHit(1.5f);

    //コライダーコンポーネントの生成
    m_Collider  = std::make_shared<OBBColliderComponent>();
    m_Collider -> SetSize({ 12.0f, 10.0f, 30.0f }); // モデルに合わせて調整
    m_Collider ->SetLocalOffset(Vector3(0.0f,0.0f ,15.0f));

  //---------------GameObjectに追加---------------
    AddComponent(modelComp);
    AddComponent(moveComp);
    AddComponent(shootComp);
    AddComponent(HPComp);
    AddComponent(m_Collider);
  //----------------------------------------------

    GameObject::Initialize();

     //初期回転(ラジアンでの設定)
    SetRotation(DirectX::SimpleMath::Vector3(0.0f, 0.0f, 0.0f));
}

void Player::Update(float dt)
{
    //コンポーネント等のUpdate
    GameObject::Update(dt);
}

void Player::OnCollision(GameObject* other)
{
    if (!other) { return; }

    //弾衝突判定
    if (auto bc = other->GetComponent<BulletComponent>())
    {
        if (bc->GetBulletType() == BulletComponent::BulletType::ENEMY)
        {
            //敵弾向こうに任せる
            auto hp = GetComponent<HitPointComponent>();
            if (hp)
            {
                DamageInfo di;
                di.amount = 2;
                di.instigator = other;
                di.tag = "enemy_bullet";
                bool applied = hp->ApplyDamage(di);
                if (applied)
                {
                    // 弾は消す
                    if (auto s = GetScene())
                    {
                        s->RemoveObject(other);
                    }
                }
            }
            else
            {
                // 互換: 旧処理（即死）
                if (auto s = GetScene())
                {
                    s->RemoveObject(this);
                    s->RemoveObject(other);
                }
            }
            return;
        }
    }

    //建物衝突処理
    if (auto b = dynamic_cast<Building*>(other))
    {
        //まずめり込みを押し出しで解消
        //ResolvePenetrationWith(other);

        //HitPointComponentにダメージを与える
        auto hp = GetComponent<HitPointComponent>();

        if (hp)
        {
            DamageInfo di;
            di.amount = 4;
            di.instigator = other;
            di.tag = "collision_building";
            hp->ApplyDamage(di);
        }
        return;
    }
}