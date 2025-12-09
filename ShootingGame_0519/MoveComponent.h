// MoveComponent.h
#pragma once
#include "Component.h"
#include "Input.h"
#include "ICameraViewProvider.h"
#include <SimpleMath.h>
#include <fstream>
#include <algorithm>
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

    //-----------------------------------Set関数関連------------------------------------
    void SetSpeed(float speed) { m_baseSpeed = speed; }
    void SetCameraView(ICameraViewProvider* camera) { m_camera = camera; }
    void SetPlayArea(PlayAreaComponent* playArea) { m_playArea = playArea; }
    /*void SetObstacleTester(std::function<bool(const DirectX::SimpleMath::Vector3&,
                            const DirectX::SimpleMath::Vector3&,
                            float,
                            DirectX::SimpleMath::Vector3&,
                            float&)> tester) { m_obstacleTester = std::move(tester); }*/
    //-----------------------------------Get関数関連------------------------------------
    bool GetBoostingState() const { return m_isBoosting; }
    
    DirectX::SimpleMath::Vector3 GetCurrentVelocity() const{ return m_velocity + m_externalVelocity; }

	//ブースト量を取得する関数(演出で使用)
    float GetBoostIntensity() const
    {
        if (m_isBoosting)
        {
			//ブースト中(0.0～1.0)
            float t = m_boostTimer / m_boostSeconds;
            return std::clamp(t, 0.0f, 1.0f);
        }
        else if (m_recoverTimer >= 0.0f)
        {
			//ブーストから回復中(1.0～0.0)
            float t = 1.0f - std::clamp(m_recoverTimer / m_boostRecover, 0.0f, 1.0f);
            return t;
        }
        else
        {
            return 0.0f;
        }
    }

    
	//-----------------------------------その他関数関連------------------------------------
    void AddImpulse(const DirectX::SimpleMath::Vector3& impulse){ m_externalVelocity += impulse; }
    void HandleCollisionCorrection(const DirectX::SimpleMath::Vector3& push,
                                   const DirectX::SimpleMath::Vector3& contactNormal);

    void ApplyCollisionPush();
private:

    //-----------------基本向き取得用--------------------
    //カメラの向きを取得する用のポインタ
    ICameraViewProvider* m_camera = nullptr;

    //--------------------回転制御-----------------------
    float m_rotateSpeed = 10.0f;  //yawの最大回転速度(ラジアン/秒)の上限
    float m_rotSmoothK  = 8.0f;   //yawの指数補間係数(大きいほど素早く回転する)

    float m_currentRoll = 0.0f;   //現在のロール
    float m_pitchSpeed  = 2.0f;   //ピッチ回転速度
    float m_currentPitch = 0.0f;  //現在のピッチ

    //-----------------ブースト関連変数--------------------
    float m_boostMultiplier = 2.5f; //何倍速くなるか
    float m_boostSeconds  = 1.0f;    //ブースト持続秒数
    float m_boostRecover  = 0.8f;    //通常速度に戻るまでの秒数
    float m_boostCooldown = 0.5f;   // 次ブーストまでのクールダウン

    bool  m_isBoosting = false;     //ブースト中かどうか
    float m_boostTimer = 0.0f;      //ブースト経過時間
    float m_recoverTimer  = 0.0f;   //回復経過時間
    float m_cooldownTimer = 0.0f;   //クールダウン残り時間

    float m_baseSpeed = 35.0f;        //元のスピード格納用
    int   m_boostKey  = VK_SHIFT;     //ブーストキー
    bool  m_prevBoostKeyDown = false; //エッジ検出用

    //------------------プレイエリア---------------------
    PlayAreaComponent* m_playArea = nullptr;

    //------------------障害物回避用変数-----------------
    /*float m_predictTimeBase = 0.12f;
    float m_predictTimeFactor = 0.18f; // predictTime = base + (speed/baseSpeed) * factor
    float m_avoidRange = 5.0f;
    float m_avoidWeight = 1.6f;
    float m_forwardWeight = 0.6f;
    float m_inputWeight = 0.9f;
    int   m_avoidSamples = 5; 

    std::function<bool(const DirectX::SimpleMath::Vector3& start,
        const DirectX::SimpleMath::Vector3& dir,
        float length,
        DirectX::SimpleMath::Vector3& outNormal,
        float& outDist)> m_obstacleTester;*/

    //---------------------速度関連----------------------
	DirectX::SimpleMath::Vector3 m_externalVelocity     //外部から力を加えられる速度
        = DirectX::SimpleMath::Vector3::Zero; 
    DirectX::SimpleMath::Vector3 m_velocity 
        = DirectX::SimpleMath::Vector3::Zero;

    //------------視覚的なドリフト(ロール用) -------------
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

    DirectX::SimpleMath::Vector3 m_accumulatedPush = DirectX::SimpleMath::Vector3::Zero;
    bool m_hadCollisionThisFrame = false;

    float m_externalDamping = 1.0f;

    //------------------衝突押し出し(今はロジック停止中) ---------------------
    DirectX::SimpleMath::Vector3 m_totalPushThisFrame = DirectX::SimpleMath::Vector3::Zero;
    bool m_hasPushThisFrame = false;

};
