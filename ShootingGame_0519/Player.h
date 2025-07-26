#pragma once
#include "GameObject.h"
#include "ModelComponent.h"
#include "MoveComponent.h"
#include "ShootingComponent.h"


class Player : public GameObject
{
public:
    Player() = default;
    ~Player() override = default;

    void Initialize() override;
    void Update() override;
};