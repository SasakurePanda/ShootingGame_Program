#include "Enemy.h"

void Enemy::Initialize()
{
    //基底クラスの初期化（Component群）
    GameObject::Initialize();

    //モデルコンポーネントの生成
    auto modelComp = std::make_shared<ModelComponent>();

    //モデルの読み込み（失敗時に備えログなども可）
    modelComp->LoadModel("Asset/Model/Robot/uploads_files_3862208_Cube.fbx");

    SetRotation(DirectX::SimpleMath::Vector3(0.0, 0.0, 0.0));

    //コライダーコンポーネントの生成
    m_Collider  = std::make_shared<OBBColliderComponent>();
    m_Collider -> SetSize({ 2.0f, 4.0f, 2.0f }); // モデルに合わせて調整
    
    //---------------GameObjectに追加---------------
    AddComponent(modelComp);
    AddComponent(m_Collider);
    //----------------------------------------------
}

void Enemy::Update(float dt)
{
    //コンポーネント等のUpdate
    GameObject::Update(dt);
}
