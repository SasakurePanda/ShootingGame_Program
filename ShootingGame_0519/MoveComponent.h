#pragma once
#include "Component.h"
#include "Input.h"
#include "ICameraViewProvider.h"
#include <SimpleMath.h>

class MoveComponent : public Component
{
public:
    MoveComponent() = default;
    ~MoveComponent() override = default;

    void Initialize() override;
    void Update(float dt) override;

    //移動速度のセット関数
    void SetSpeed(float speed) { m_speed = speed; }

    //移動の前後左右を決めるために使うカメラのセット関数
    void SetCameraView(ICameraViewProvider* camera) { m_camera = camera; }

private: 
    //ユニット/秒
    float m_speed = 7.5f;

    //カメラの向きを取得する用のポインタ
    ICameraViewProvider* m_camera = nullptr;
    
    float m_rotateSpeed = 7.5f; // rad/s: 調整可（2.5〜4.0 を推奨）

    float m_currentRoll = 0.0f; // 現在のロール（member に変更）
};
