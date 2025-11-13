#include "Player.h"
#include "BulletComponent.h"

void Player::Initialize()
{
    //基底クラスの初期化（Component群）
    GameObject::Initialize(); 

    //モデルコンポーネントの生成
    auto modelComp = std::make_shared<ModelComponent>();

    //移動コンポーネントの生成
    auto moveComp = std::make_shared<MoveComponent>();
    moveComp->Initialize();

    //弾発射コンポーネントの生成
    auto shootComp = std::make_shared<ShootingComponent>();

    //コライダーコンポーネントの生成
    m_Collider  = std::make_shared<OBBColliderComponent>();
    m_Collider -> SetSize({ 12.0f, 10.0f, 30.0f }); // モデルに合わせて調整

    //モデルの読み込み（失敗時に備えログなども可）
    modelComp->LoadModel("Asset/Model/Robot/12211_Robot_l2.obj");

    if (modelComp)
    {
        // 赤色に変更（Color(r,g,b,a)）
        modelComp->SetColor(Color(1.0f, 0.0f, 0.0f, 1.0f));
    }

  //---------------GameObjectに追加---------------
    AddComponent(modelComp);
    AddComponent(moveComp);
    AddComponent(shootComp);
    AddComponent(m_Collider);
  //----------------------------------------------

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
    //相手が弾かどうか
    if (auto bc = other->GetComponent<BulletComponent>()) 
    {
        //弾が敵のものかどうか
        if (bc->GetBulletType() == BulletComponent::BulletType::ENEMY)
        {
            //Enemy弾に当たったらそのPlayerも弾も削除
            if (auto scene = GetScene())
            {
                //今後HP処理を入れる
                scene->RemoveObject(this);
                scene->RemoveObject(other);
            }
        }
    }
}