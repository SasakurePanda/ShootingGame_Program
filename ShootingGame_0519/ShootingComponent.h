#pragma once
#include "Component.h"
#include "Bullet.h"
#include "Input.h"
#include "IScene.h"
#include "ICameraViewProvider.h"

class ShootingComponent : public Component
{
public:
    void Update(float dt) override;

    void SetScene(IScene* scene) { m_scene = scene; }
    void SetCamera(ICameraViewProvider* camera) { m_camera = camera; } // Å©í«â¡ÅI

private:
    IScene* m_scene = nullptr;
    ICameraViewProvider* m_camera = nullptr;

    float m_cooldown = 0.3f;
    float m_timer = 0.0f;
};


