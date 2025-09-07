#pragma once
#include "GameObject.h"
#include "ModelComponent.h"
#include "MoveComponent.h"
#include "ShootingComponent.h"
#include "AABBColliderComponent.h"
#include "OBBColliderComponent.h"

class Enemy : public GameObject
{
public:
    Enemy() = default;
    ~Enemy() override = default;

    void Initialize() override;
    void Update(float dt) override;
private:
    std::shared_ptr<OBBColliderComponent> m_Collider;
};

