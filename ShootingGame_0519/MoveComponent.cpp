#include "MoveComponent.h"
#include "PlayAreaComponent.h"
#include "GameObject.h"
#include "Application.h"
#include <iostream>

using namespace DirectX::SimpleMath;

constexpr float XM_PI  = 3.14159265f;
constexpr float XM_2PI = 6.28318530f;

void MoveComponent::Initialize()
{
    
}

void MoveComponent::Uninit()
{
    if (m_dbgCsv.is_open()) { m_dbgCsv.close(); }
}

void MoveComponent::Update(float dt)
{
    if(!m_camera){ return; }

    //現時点での回転と位置を保存しておく
    Vector3 pos = GetOwner()->GetPosition();
    Vector3 rot = GetOwner()->GetRotation();
    float currentYaw = rot.y;
    float currentPitch = m_currentPitch;

    //ブースト用の入力取得
    bool keyDown = Input::IsKeyDown(m_boostKey);

    bool startBoost = false;
    if(keyDown && !m_prevBoostKeyDown && m_cooldownTimer <= 0.0f && !m_isBoosting)
    {
        startBoost = true;
    }

    m_prevBoostKeyDown = keyDown;

    if(startBoost)
    {
        //ブースト中にする
        m_isBoosting = true;

        //向きをロックにしておく
        m_lockDuringBoost = true;
        m_lockedYaw = currentYaw;
        m_lockedPitch = currentPitch;

        //ブーストのタイマーを初期化
        m_boostTimer = 0.0f;

        //ブースト回復のタイマーを初期化
        m_recoverTimer = -1.0f;

        m_cooldownTimer = m_boostCooldown;

        if (m_camera)
        {
            m_camera->SetBoostState(true); // カメラにブースト開始を通知
        }
    }

    //-----------------ブースト時間経過判定---------------------

    if(m_isBoosting)
    {
        m_boostTimer += dt;
        if (m_boostTimer >= m_boostSeconds)
        {
            m_isBoosting = false;
            m_recoverTimer = 0.0f; //回復フェーズ開始

            //向きのロック解除
            m_lockDuringBoost = false;
            if (m_camera)
            {
                m_camera->SetBoostState(false); // カメラにブースト終了を通知
            }
        }
    }

    //-----------------クールダウン減算---------------------
    if (m_cooldownTimer > 0.0f)
    {
        m_cooldownTimer -= dt;
        if (m_cooldownTimer < 0.0f)
        {
            m_cooldownTimer = 0.0f;
        }
    }

    //-----------------速度決定---------------------
    float currentSpeed = m_baseSpeed;
    if (m_isBoosting)
    {
        //ブースト中は速さに倍率を付ける
        currentSpeed = m_baseSpeed * m_boostMultiplier;

    }
    else if (m_recoverTimer >= 0.0f && m_recoverTimer < m_boostRecover)
    {
        m_recoverTimer += dt;
        float t = std::clamp(m_recoverTimer / m_boostRecover, 0.0f, 1.0f);
        float ease = 1.0f - (1.0f - t) * (1.0f - t);
        float currentMultiplier = 1.0f + (m_boostMultiplier - 1.0f) * (1.0f - ease);
        currentSpeed = m_baseSpeed * currentMultiplier;
    }

    //-----------------通常移動処理---------------------
    Vector3 aimTarget = m_camera->GetAimPoint();

    Vector3 toTarget = aimTarget - pos;
    if (toTarget.LengthSquared() < 1e-6f)
    {
        Vector3 forward = Vector3(std::sin(currentYaw) * std::cos(currentPitch),
            std::sin(currentPitch),
            std::cos(currentYaw) * std::cos(currentPitch));
        forward.Normalize();
        pos += forward * currentSpeed * dt;
        GetOwner()->SetPosition(pos);
        return;
    }

    Vector3 toDir;
    float targetYaw = 0.0f;
    float targetPitch = 0.0f;

    if (m_isBoosting && m_lockDuringBoost)
    {
        // ブースト開始時に保存した yaw/pitch から向きを計算
        targetYaw = m_lockedYaw;
        targetPitch = m_lockedPitch;
        toDir = Vector3(std::sin(targetYaw) * std::cos(targetPitch),
            std::sin(targetPitch),
            std::cos(targetYaw) * std::cos(targetPitch));
        toDir.Normalize();
    }
    else
    {
        toDir = toTarget;
        toDir.Normalize();

        targetYaw = std::atan2(toDir.x, toDir.z);
        float horiz = std::sqrt(toDir.x * toDir.x + toDir.z * toDir.z);
        targetPitch = std::atan2(toDir.y, horiz);
    }

    float deltaYaw = targetYaw - currentYaw;
    while (deltaYaw > XM_PI) 
    { 
        deltaYaw -= XM_2PI;
    }
    while (deltaYaw < -XM_PI)
    { 
        deltaYaw += XM_2PI;
    }

    float deltaPitch = targetPitch - currentPitch;
    while (deltaPitch > XM_PI) 
    { 
        deltaPitch -= XM_2PI;
    }
    while (deltaPitch < -XM_PI)
    { 
        deltaPitch += XM_2PI;
    }

    //ブースト中なら回転や向きの変更は行わない
    float appliedYaw = 0.0f;
    float appliedPitch = 0.0f;
    if (m_isBoosting && m_lockDuringBoost)
    {
        //何もしない
        currentYaw = m_lockedYaw;
        currentPitch = m_lockedPitch;
    }
    else
    {
        //通常の回転や向きの変更
        float maxYawTurn = m_rotateSpeed * dt;
        appliedYaw = std::clamp(deltaYaw, -maxYawTurn, maxYawTurn);
        currentYaw += appliedYaw;

        float maxPitchTurn = m_pitchSpeed * dt;
        appliedPitch = std::clamp(deltaPitch, -maxPitchTurn, maxPitchTurn);
        currentPitch += appliedPitch;
    }

    Vector3 currentForward = Vector3(
        std::sin(currentYaw) * std::cos(currentPitch),
        std::sin(currentPitch),
        std::cos(currentYaw) * std::cos(currentPitch)
    );

    if (currentForward.LengthSquared() > 1e-6f)
    { 
        currentForward.Normalize();
    }
    else
    {
        currentForward = Vector3(0, 0, 1);
    }

    //ロール回転はブースト中でも起動しておく
    Vector3 cross = currentForward.Cross(toDir);
    float lateral = cross.y;
    constexpr float DegToRad = 3.14159265358979323846f / 180.0f;
    float maxBankAngle = 20.0f * DegToRad;
    float desiredRoll = -lateral * maxBankAngle;
    float bankSmooth = 6.0f;
    m_currentRoll = m_currentRoll + (desiredRoll - m_currentRoll) * std::min(1.0f, bankSmooth * dt);

    rot.x = -currentPitch;
    rot.y = currentYaw;
    rot.z = m_currentRoll;
    GetOwner()->SetRotation(rot);

    //前進処理
    pos += currentForward * currentSpeed * dt;

    //PlayAreaがあるなら当てはめ（オプション）
    if (m_playArea)
    {
        pos = m_playArea->ResolvePosition(GetOwner()->GetPosition(), pos);
    }
    GetOwner()->SetPosition(pos);

    m_currentPitch = currentPitch;
}


