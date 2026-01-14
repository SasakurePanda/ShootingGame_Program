#pragma once
#include "Component.h"
#include "Input.h"
#include "ICameraViewProvider.h"
#include "IMovable.h"
#include <SimpleMath.h>
#include <algorithm>
#include <functional>

class PlayAreaComponent;

class MoveComponent : public Component, public IMovable
{
public:

    MoveComponent() = default;
    ~MoveComponent() override = default;

    void Initialize() override;
    void Update(float dt) override;
    void Uninit() override;

    //-------------Set関数--------------
    void SetSpeed(float speed) { m_baseSpeed = speed; }
    void SetCameraView(ICameraViewProvider* camera) { m_camera = camera; }
    void SetPlayArea(PlayAreaComponent* playArea) { m_playArea = playArea; }
    void SetBoostKey(int vk) { m_boostKey = vk; }
    void SetVelocity(const DirectX::SimpleMath::Vector3& velocity) override
    {
        m_velocity = velocity;
        m_externalVelocity = DirectX::SimpleMath::Vector3::Zero;
    }

    //-------------Get関数--------------
    bool GetBoostingState() const { return m_isBoosting; }
    DirectX::SimpleMath::Vector3 GetCurrentVelocity() const { return m_velocity + m_externalVelocity; }
    //ブースト量を取得する関数(演出で使用)
    float GetBoostIntensity() const;
    DirectX::SimpleMath::Vector3 GetVelocity() const override { return m_velocity + m_externalVelocity; }

    //-----------------------------------その他関数関連------------------------------------
    void AddImpulse(const DirectX::SimpleMath::Vector3& impulse) { m_externalVelocity += impulse; }

    //削除予定
    void HandleCollisionCorrection(const DirectX::SimpleMath::Vector3& push,
        const DirectX::SimpleMath::Vector3& contactNormal);

    //削除予定
    void ApplyCollisionPush();

	//-------------------------入力取得関連------------------------
    void RequestBoost();

private:

    //-----------------カメラ参照関連--------------------
    ICameraViewProvider* m_camera = nullptr;

    //------------------回転制御関連---------------------
    float m_rotateSpeed  = 10.0f;  //yawの最大回転速度(ラジアン/秒)の上限
    float m_rotSmoothK   = 8.0f;    //yawの指数補間係数(大きいほど素早く回転する)
    float m_currentRoll  = 0.0f;   //現在のロール
    float m_pitchSpeed   = 2.0f;   //ピッチ回転速度
    float m_currentPitch = 0.0f;  //現在のピッチ

    //------------------ブースト関連----------------------
    float m_boostMultiplier = 2.5f; //何倍速くなるか
    float m_boostSeconds    = 1.0f; //ブースト持続秒数
    float m_boostRecover    = 0.8f; //通常速度に戻るまでの秒数
    float m_boostCooldown   = 0.5f; // 次ブーストまでのクールダウン

    bool  m_isBoosting    = false;  //ブースト中かどうか
    float m_boostTimer    = 0.0f;   //ブースト経過時間
    float m_recoverTimer  = 0.0f;   //回復経過時間
    float m_cooldownTimer = 0.0f;   //クールダウン残り時間

    float m_baseSpeed = 35.0f;        //元のスピード格納用
    int   m_boostKey  = VK_SHIFT;     //ブーストキー
    bool  m_prevBoostKeyDown = false; //エッジ検出用

    //----------------プレイエリア関連-------------------
    PlayAreaComponent* m_playArea = nullptr;

    //---------------------速度関連----------------------
    DirectX::SimpleMath::Vector3 m_externalVelocity = DirectX::SimpleMath::Vector3::Zero; //外部から力を加えられる速度
    DirectX::SimpleMath::Vector3 m_velocity         = DirectX::SimpleMath::Vector3::Zero;

    //-----------------視覚的なドリフ関連-----------------
    float m_prevYaw = 0.0f;            // 前フレームの yaw（角速度計算用）
    float m_visualPitchTilt = 0.0f;    // 視覚用のピッチ傾き（ノーズ上下）

    //---------------チューニングパラメータ----------------
    float m_rollYawFactor     = 0.6f;  // ヨー速度からのロール寄与係数
    float m_rollLateralFactor = 0.9f;  // lateral（cross.y）からの寄与係数
    float m_rollSpeedScale    = 1.0f;  // 速度によるスケール（1.0＝無変化）
    float m_rollLerpK     = 8.0f;        // ロールの滑らかさ係数（大きいほど速く追従）
    float m_maxVisualRoll = 0.38397244f; // 最大ロール角（ラジアン、≈22度）


    float m_verticalTiltFactor = 0.4f;     // 垂直速度からのピッチ寄与（ラジアンスケール）
    float m_maxVerticalTilt    = 0.13962634f; // 最大ピッチ傾き（ラジアン、≈8度）

    float m_pitchTiltSmoothK       = 6.0f;         // 指数ローパスの速さ（小さくするとゆっくり）
    float m_maxPitchDeltaDegPerSec = 60.0f;  // 1秒あたりの最大変化量（度/秒）
    float m_pitchSaturationFactor  = 6.0f;    // atan の分母などに使う（垂直速度のスケール）

    //--------------衝突・押し出し関連------------------
    //削除予定
    DirectX::SimpleMath::Vector3 m_accumulatedPush = DirectX::SimpleMath::Vector3::Zero;
    //削除予定
    bool m_hadCollisionThisFrame = false;
    //削除予定
    DirectX::SimpleMath::Vector3 m_totalPushThisFrame = DirectX::SimpleMath::Vector3::Zero;
    //削除予定
    bool m_hasPushThisFrame = false;

    float m_externalDamping = 6.0f;   
};
