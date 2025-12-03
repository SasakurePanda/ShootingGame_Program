// MoveComponent.h
#pragma once
#include "Component.h"
#include "Input.h"
#include "ICameraViewProvider.h"
#include <SimpleMath.h>
#include <fstream>
#include <functional>

class PlayAreaComponent;

class MoveComponent : public Component
{
public:
    MoveComponent() = default;
    ~MoveComponent() override = default;

    void Initialize() override;
    void Update(float dt) override;
    void Uninit() override;

    //移動速度のセット関数
    void SetSpeed(float speed) { m_baseSpeed = speed; }

    //移動の前後左右を決めるために使うカメラのセット関数
    void SetCameraView(ICameraViewProvider* camera) { m_camera = camera; }

    void SetPlayArea(PlayAreaComponent* playArea) { m_playArea = playArea; }

    void SetObstacleTester(std::function<bool(const DirectX::SimpleMath::Vector3&,
        const DirectX::SimpleMath::Vector3&,
        float,
        DirectX::SimpleMath::Vector3&,
        float&)> tester)
    {
        m_obstacleTester = std::move(tester);
    }
 
    void AddImpulse(const DirectX::SimpleMath::Vector3& impulse)
    {
        m_externalVelocity += impulse;
    }

    void HandleCollisionCorrection(const DirectX::SimpleMath::Vector3& push,
        const DirectX::SimpleMath::Vector3& contactNormal);

    void ClearCollisionCorrectionFlag() { m_collisionCorrectedThisFrame = false; }

private:

    //カメラの向きを取得する用のポインタ
    ICameraViewProvider* m_camera = nullptr;

    float m_rotateSpeed = 10.0f;  //（未使用のまま残すが、角速度上限として保有）
    float m_rotSmoothK = 8.0f;    // 回転の指数遅延係数（大きいほど素早く追従）

    float m_currentRoll = 0.0f;   //現在のロール

    float m_pitchSpeed = 3.5f;    //ピッチ回転速度（代替の制限用に残す）

    float m_currentPitch = 0.0f;  //現在のピッチ

    //-------------------------ブースト関連変数----------------------------
    float m_boostMultiplier = 2.5f;   // 何倍速くなるか

    float m_boostSeconds = 1.0f;   // ブーストが持続する秒数

    float m_boostRecover = 0.8f;      //通常速度に戻るまでの秒数

    float m_boostCooldown = 0.5f;     // 次ブーストできるまでのクールダウン

    bool  m_isBoosting = false;       //ブースト中かどうか 

    float m_boostTimer = 0.0f;        // ブースト経過

    float m_recoverTimer = 0.0f;      // 回復の進捗

    float m_cooldownTimer = 0.0f;

    float m_baseSpeed = 35.0f;        // 元のスピード格納用

    int   m_boostKey = VK_SHIFT;      //トリガーキー(変更可)

    bool  m_prevBoostKeyDown = false; //エッジ検出用

    PlayAreaComponent* m_playArea = nullptr;

    //-------------------------障害物回避用変数-------------------------
    float m_predictTimeBase = 0.12f;
    float m_predictTimeFactor = 0.18f; // predictTime = base + (speed/baseSpeed) * factor
    float m_avoidRange = 5.0f;
    float m_avoidWeight = 1.6f;
    float m_forwardWeight = 0.6f;
    float m_inputWeight = 0.9f;
    int   m_avoidSamples = 5; 

    std::function<bool(const DirectX::SimpleMath::Vector3& /*start*/,
        const DirectX::SimpleMath::Vector3& /*dir*/,
        float /*length*/,
        DirectX::SimpleMath::Vector3& /*outNormal*/,
        float& /*outDist*/)> m_obstacleTester;

	DirectX::SimpleMath::Vector3 m_externalVelocity = DirectX::SimpleMath::Vector3::Zero; //外部から力を加えられる速度
    DirectX::SimpleMath::Vector3 m_velocity = DirectX::SimpleMath::Vector3::Zero;

    bool m_collisionCorrectedThisFrame;

    // --- 視覚的なドリフト用メンバ ---
    float m_prevYaw = 0.0f;            // 前フレームの yaw（角速度計算用）
    float m_visualPitchTilt = 0.0f;    // 視覚用のピッチ傾き（ノーズ上下）

    // チューニングパラメータ（必要なら IMGUI で変更可能に）
    float m_rollYawFactor = 0.6f;      // ヨー速度からのロール寄与係数
    float m_rollLateralFactor = 0.9f;  // lateral（cross.y）からの寄与係数
    float m_rollSpeedScale = 1.0f;     // 速度によるスケール（1.0＝無変化）
    float m_rollLerpK = 8.0f;          // ロールの滑らかさ係数（大きいほど速く追従）
    float m_maxVisualRoll = 0.38397244f; // 最大ロール角（ラジアン、≈22度）
    float m_verticalTiltFactor = 0.4f; // 垂直速度からのピッチ寄与（ラジアンスケール）
    float m_maxVerticalTilt = 0.13962634f; // 最大ピッチ傾き（ラジアン、≈8度）

    float m_pitchTiltSmoothK = 6.0f;         // 指数ローパスの速さ（小さくするとゆっくり）
    float m_maxPitchDeltaDegPerSec = 60.0f;  // 1秒あたりの最大変化量（度/秒）
    float m_pitchSaturationFactor = 6.0f;    // atan の分母などに使う（垂直速度のスケール）

};
