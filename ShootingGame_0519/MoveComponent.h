#pragma once
#include "Component.h"
#include "Input.h"
#include "ICameraViewProvider.h"
#include <SimpleMath.h>
#include <fstream>

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
    void SetSpeed(float speed) { m_speed = speed; }

    //移動の前後左右を決めるために使うカメラのセット関数
    void SetCameraView(ICameraViewProvider* camera) { m_camera = camera; }

    void SetPlayArea(PlayAreaComponent* playArea) { m_playArea = playArea; }

private: 
    std::ofstream m_dbgCsv;
    int m_frameCounter = 0;

    //スピード/秒
    float m_speed = 45.0f;

    //カメラの向きを取得する用のポインタ
    ICameraViewProvider* m_camera = nullptr;
    
    float m_rotateSpeed = 10.0f;  //回転速度

    float m_currentRoll = 0.0f;   //現在のロール

    float m_pitchSpeed = 3.5f;    //ピッチ回転速度

    float m_currentPitch = 0.0f;  //現在のピッチ

    //-------------------------ブースト関連の変数----------------------------
    float m_boostMultiplier = 2.5f;   // 何倍速くなるか

    float m_boostSeconds    = 1.0f;   // ブーストが持続する秒数

    float m_boostRecover = 0.8f;      //通常速度に戻るまでの秒数

    float m_boostCooldown = 0.5f;     // 次ブーストできるまでのクールダウン

    bool  m_isBoosting = false;       //ブースト中かどうか 

    float m_boostTimer = 0.0f;        // ブースト経過

    float m_recoverTimer = 0.0f;      // 回復の進捗

    float m_cooldownTimer = 0.0f;

    float m_baseSpeed = 25.0f;        // 元のスピード格納用

    int   m_boostKey = VK_SHIFT;      //トリガーキー(変更可)

    bool  m_prevBoostKeyDown = false; //エッジ検出用

    bool  m_lockDuringBoost = false;   // ブースト中は向きをロックするフラグ

    float m_lockedYaw = 0.0f;          // ブースト開始時の yaw を保持

    float m_lockedPitch = 0.0f;        // ブースト開始時の pitch を保持

    PlayAreaComponent* m_playArea = nullptr;
};


