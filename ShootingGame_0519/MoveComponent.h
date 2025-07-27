#pragma once
#include "Component.h"
#include "Input.h"
#include "ICameraViewProvider.h"
#include <SimpleMath.h>

// 前方・右方向に移動するためのコンポーネント
class MoveComponent : public Component
{
public:
    MoveComponent() = default;
    ~MoveComponent() override = default;

    void Initialize() override;

    void Update() override;

    void SetSpeed(float speed) { m_speed = speed; }
    void SetCameraView(ICameraViewProvider* camera) { m_camera = camera; }

private:
    float m_speed = 5.0f; // 単位: ユニット/秒
    ICameraViewProvider* m_camera = nullptr; //カメラの前向き、右向きベクトルの取得用インターフェイス
};

