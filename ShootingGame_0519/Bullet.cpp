#include "Bullet.h"

void Bullet::Initialize()
{
        auto model = std::make_shared<ModelComponent>();
        model->LoadModel("Asset/Model/Bullet/golf_ball.obj");

        auto bullet = std::make_shared<BulletComponent>();
        bullet->Initialize();

        //AddComponent(model);
        AddComponent(bullet);
}

void Bullet::Update(float dt)
{
    GameObject::Update(dt);
}