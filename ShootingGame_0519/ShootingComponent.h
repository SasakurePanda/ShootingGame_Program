#pragma once
#include "Component.h"
#include "Bullet.h"
#include "Input.h"
#include "IScene.h"

class ShootingComponent : public Component
{
public:
    void Update() override;

    void SetScene(IScene* scene)
    {
        if (scene != nullptr)
        {
            m_scene = scene;
        }
    }

private:
    IScene* m_scene = nullptr;

    float m_cooldown = 0.3f;    // クールタイム（秒）
    float m_timer = 0.0f;       // 経過時間（秒）
};

