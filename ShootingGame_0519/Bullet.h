#pragma once
#include "GameObject.h"
#include "ModelComponent.h"
#include "BulletComponent.h"

class Bullet : public GameObject
{
public:
    void Initialize() override;
    void Update(float dt) override;
};

