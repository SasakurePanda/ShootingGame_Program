#include "Player.h"

void Player::Initialize()
{
    //基底クラスの初期化（Component群）
    GameObject::Initialize(); 

    //モデルコンポーネントの生成
    auto modelComp = std::make_shared<ModelComponent>();

    //移動コンポーネントの生成
    auto moveComp = std::make_shared<MoveComponent>();

    //弾発射コンポーネントの生成
    auto shootComp = std::make_shared<ShootingComponent>();

    //コライダーコンポーネントの生成
    m_Collider  = std::make_shared<OBBColliderComponent>();
    m_Collider -> SetSize({ 12.0f, 4.0f, 30.0f }); // モデルに合わせて調整
 

    //モデルの読み込み（失敗時に備えログなども可）
    modelComp->LoadModel("Asset/Model/Robot/12211_Robot_l2.obj");

    SetRotation(DirectX::SimpleMath::Vector3(14.5,0.0,3.1));
   
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
