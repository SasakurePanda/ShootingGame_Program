#include <iostream>
#include "DebugPlayer.h"
#include "ModelComponent.h"

void DebugPlayer::Initialize()
{
    auto move = std::make_shared<DebugMoveComponent>();

    //モデルコンポーネントの生成
    auto model = std::make_shared<ModelComponent>();
    //モデルの読み込み
    model->LoadModel("Asset/Model/Player/Fighterjet.obj");
    model->SetColor(Color(1, 0, 0, 1));

    AddComponent(move);
    AddComponent(model);

    GameObject::Initialize();
}

void DebugPlayer::Update(float dt)
{
    GameObject::Update(dt);
}

void DebugPlayer::Draw(float dt)
{
    GameObject::Draw(dt);
}