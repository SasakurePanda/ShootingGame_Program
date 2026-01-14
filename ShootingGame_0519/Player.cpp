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
#include "Enemy.h"
#include "PushOutComponent.h"
#include <iostream>

void Player::Initialize()
{
    //基底クラスの初期化（Component群）
    GameObject::Initialize(); 

    //モデルコンポーネントの生成
    auto modelComp = std::make_shared<ModelComponent>();
    //モデルの読み込み
    modelComp->LoadModel("Asset/Model/Player/Fighterjet.obj");
    modelComp->SetColor(Color(1, 0, 0, 1));

    //移動コンポーネントの生成
    auto moveComp = std::make_shared<MoveComponent>();
    //弾発射コンポーネントの生成
    auto shootComp = std::make_shared<ShootingComponent>();
    //HPコンポーネントの生成
    auto HPComp = std::make_shared<HitPointComponent>(20);
    HPComp->SetInvincibilityOnHit(1.5f);

    auto push = std::make_shared<PushOutComponent>();
    push->SetMass(2.0f);

    //コライダーコンポーネントの生成
    m_Collider = std::make_shared<OBBColliderComponent>();
    m_Collider -> SetSize({ 6.0f, 1.5f, 8.0f }); // モデルに合わせて調整
    //m_Collider -> SetLocalOffset(Vector3(0.0f,0.0f ,15.0f));
    m_Collider ->isStatic = false;

  //---------------GameObjectに追加---------------
    AddComponent(modelComp);
    AddComponent(moveComp);
    AddComponent(shootComp);
    AddComponent(HPComp);
    AddComponent(m_Collider);
    AddComponent(push);
  //----------------------------------------------
    
    SetPosition({ 0.0f, 0.0f, 125.0f });
    SetRotation({ 0.0,60.0,0.0 });
    SetScale({ 1.0f, 1.0f, 1.0f });
    GameObject::Initialize();

    
}

void Player::Update(float dt)
{
    //コンポーネント等のUpdate
    GameObject::Update(dt);
}

void Player::OnCollision(GameObject* other)
{
    if (!other) { return; }

    //--------弾の判定処理-----------
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

    //--------建物衝突処理--------
    if (auto b = dynamic_cast<Building*>(other))
    {
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

    //--------敵衝突処理--------
    if (auto b = dynamic_cast<Enemy*>(other))
    {
        auto hp = GetComponent<HitPointComponent>();
        if (hp)
        {
            DamageInfo di;
            di.amount = 4;
            di.instigator = other;
            di.tag = "collision_enemy";
            bool applied = hp->ApplyDamage(di);
            if (applied)
            {
                // 水平面での方向ベクトル（player <- other）
                DirectX::SimpleMath::Vector3 dir = GetPosition() - other->GetPosition();
                dir.y = 0.0f;
                if (dir.LengthSquared() < 1e-6f)
                {
                    // 位置がほぼ被っている場合はランダムに左右を選ぶ
                    int r = std::rand() % 2;
                    if (r == 0) 
                    { 
                        dir = DirectX::SimpleMath::Vector3(1.0f, 0.0f, 0.0f);
                    }
                    else
                    { 
                        dir = DirectX::SimpleMath::Vector3(-1.0f, 0.0f, 0.0f);
                    }
                }
                dir.Normalize();

                // dir に直交する水平の左方向ベクトルを作る（-z,0,x を利用）
                DirectX::SimpleMath::Vector3 lateral = DirectX::SimpleMath::Vector3(-dir.z, 0.0f, dir.x);
                lateral.Normalize();

                // 少しだけ左右に弾く。符号はランダムor衝突の位置差で決める（ここはランダム）
                float sign = (std::rand() % 2 == 0) ? 1.0f : -1.0f;

                // インパルスの大きさ（チューニング）
                const float impulseStrength = 3.0f; // とても小さい: 調整可
                DirectX::SimpleMath::Vector3 impulse = lateral * sign * impulseStrength;

                // MoveComponent にインパルスを与える
                auto mv = GetComponent<MoveComponent>();
                if (mv)
                {
                    mv->AddImpulse(impulse);
                }
                else
                {
                    // 万が一 MoveComponent が無ければ位置をわずかに移動（フォールバック）
                    DirectX::SimpleMath::Vector3 pos = GetPosition();
                    pos += impulse * 0.05f;
                    SetPosition(pos);
                }

                // 見栄え用（ヒットストップ等）を入れるならここで呼ぶ
                if (auto s = GetScene())
                {
                    // s->ApplyHitStop(0.05f); // 実装があれば呼ぶ
                }
            }
        }

        return;
    }
}