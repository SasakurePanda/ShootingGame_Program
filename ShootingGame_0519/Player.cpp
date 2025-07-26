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

    //モデルの読み込み（失敗時に備えログなども可）
    modelComp->LoadModel("Asset/Model/Robot/12211_Robot_l2.obj");

    SetRotation(DirectX::SimpleMath::Vector3(14.5,0.0,3.1));
   
  //---------------GameObjectに追加---------------
    AddComponent(modelComp);
    AddComponent(moveComp);
    AddComponent(shootComp);
  //----------------------------------------------
}

void Player::Update()
{
    //コンポーネント等のUpdate
    GameObject::Update();
}
