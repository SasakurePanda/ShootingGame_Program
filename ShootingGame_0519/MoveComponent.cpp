#include "MoveComponent.h"
#include "PlayAreaComponent.h"
#include "GameObject.h"
#include "Application.h"
#include <iostream>
#include <cmath>
#include <algorithm>

using namespace DirectX::SimpleMath;

static float LerpExpFactor(float k, float dt)
{
    //
    return 1.0f - std::exp(-k * dt);
}

//
static float NormalizeAngleDelta(float a)
{
    while (a > XM_PI) a -= XM_2PI;
    while (a < -XM_PI) a += XM_2PI;
    return a;
}

void MoveComponent::Initialize()
{
    m_prevYaw = GetOwner() ? GetOwner()->GetRotation().y : 0.0f;
    m_visualPitchTilt = 0.0f;
}

void MoveComponent::Uninit()
{

}

// MoveComponent.cpp (Update の差し替え)
void MoveComponent::Update(float dt)
{
    // カメラがなければ処理しない
    if (!m_camera)
    {
        return;
    }

    // 現時点でのPlayerの位置・回転取得
    Vector3 pos = GetOwner()->GetPosition();
    Vector3 rot = GetOwner()->GetRotation();

    // 現時点でのyaw/pitch取得
    float currentYaw = rot.y;
    float currentPitch = m_currentPitch;

    // ブースト用の入力取得
    bool keyDown = Input::IsKeyPressed(m_boostKey);

    // -----------------ブースト開始判定---------------------
    bool startBoost = false;
    if (keyDown && !m_prevBoostKeyDown && m_cooldownTimer <= 0.0f && !m_isBoosting)
    {
        startBoost = true;
    }
    m_prevBoostKeyDown = keyDown;

    if (startBoost)
    {
        m_isBoosting = true;
        m_boostTimer = 0.0f;
        m_recoverTimer = -1.0f;
        m_cooldownTimer = m_boostCooldown;
        if (m_camera)
        {
            m_camera->SetBoostState(true);
        }
    }

    if (m_isBoosting)
    {
        m_boostTimer += dt;
        if (m_boostTimer >= m_boostSeconds)
        {
            m_isBoosting = false;
            m_recoverTimer = 0.0f;
            if (m_camera)
            {
                m_camera->SetBoostState(false);
            }
        }
    }

    if (m_cooldownTimer > 0.0f)
    {
        m_cooldownTimer -= dt;
        if (m_cooldownTimer < 0.0f) m_cooldownTimer = 0.0f;
    }

    // -----------------速度決定---------------------
    float currentSpeed = m_baseSpeed;
    if (m_isBoosting)
    {
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

    // -----------------通常移動処理---------------------
    Vector3 aimTarget = m_camera->GetAimPoint();
    Vector3 toTarget = aimTarget - pos;

    // 目標点が近い場合は現在向きのまま前進して終わる（早期リターン）
    if (toTarget.LengthSquared() < 1e-6f)
    {
        Vector3 forward = Vector3(std::sin(currentYaw) * std::cos(currentPitch),
            std::sin(currentPitch),
            std::cos(currentYaw) * std::cos(currentPitch));
        forward.Normalize();
        // velocityベースで統一
        Vector3 desiredVel = forward * currentSpeed + m_externalVelocity;
        m_velocity = desiredVel;
        pos += m_velocity * dt;
        // PlayArea 補正
        if (m_playArea)
        {
            pos = m_playArea->ResolvePosition(GetOwner()->GetPosition(), pos);
        }
        else
        {
            if (pos.y < -1.0f) pos.y = -1.0f;
        }
        GetOwner()->SetPosition(pos);
        m_currentPitch = currentPitch;
        return;
    }

    // 現在の前方ベクトル計算
    Vector3 currentForward = Vector3(
        std::sin(currentYaw) * std::cos(currentPitch),
        std::sin(currentPitch),
        std::cos(currentYaw) * std::cos(currentPitch)
    );
    if (currentForward.LengthSquared() > 1e-6f) currentForward.Normalize();
    else currentForward = Vector3(0, 0, 1);

    // 未来位置予測（障害物回避に使用）
    float speedRatio = (m_baseSpeed > 0.0001f) ? (currentSpeed / m_baseSpeed) : 1.0f;
    float predictTime = m_predictTimeBase + speedRatio * m_predictTimeFactor;
    float predictDist = currentSpeed * predictTime;
    Vector3 futurePos = pos + currentForward * predictDist;

    // 障害物回避サンプリング
    Vector3 avoidDir = Vector3::Zero;
    if (m_obstacleTester && m_avoidSamples > 0)
    {
        const float maxAngleRad = 30.0f * XM_PI / 180.0f;
        for (int i = 0; i < m_avoidSamples; ++i)
        {
            float t = (m_avoidSamples == 1) ? 0.0f : float(i) / float(m_avoidSamples - 1);
            float sampleAngle = -maxAngleRad + t * (2.0f * maxAngleRad);
            float sampleYaw = currentYaw + sampleAngle;
            Vector3 sampleDir = Vector3(
                std::sin(sampleYaw) * std::cos(currentPitch),
                std::sin(currentPitch),
                std::cos(sampleYaw) * std::cos(currentPitch)
            );
            sampleDir.Normalize();

            float rayLen = m_avoidRange + predictDist * 0.5f;
            Vector3 hitNormal;
            float hitDist = 0.0f;
            bool hit = m_obstacleTester(futurePos, sampleDir, rayLen, hitNormal, hitDist);
            if (hit)
            {
                float strength = std::clamp((rayLen - hitDist) / rayLen, 0.0f, 1.0f);
                Vector3 away;
                if (hitNormal.LengthSquared() > 0.0001f) away = hitNormal;
                else away = -sampleDir;
                away.y = std::clamp(away.y, -0.5f, 0.5f);
                away.Normalize();
                avoidDir += away * (strength * strength);
            }
        }
        if (avoidDir.LengthSquared() > 1e-6f) avoidDir.Normalize();
    }

    // 目的方向・合成
    Vector3 toDir = toTarget; toDir.Normalize();
    Vector3 desired = toDir * m_inputWeight + currentForward * m_forwardWeight + avoidDir * m_avoidWeight;
    if (desired.LengthSquared() > 1e-6f) desired.Normalize();
    else desired = currentForward;

    // 目標角度・滑らかな回転（既存アルゴリズムを維持）
    float targetYaw = std::atan2(desired.x, desired.z);
    float horiz = std::sqrt(desired.x * desired.x + desired.z * desired.z);
    float targetPitch = std::atan2(desired.y, horiz);
    float deltaYaw = NormalizeAngleDelta(targetYaw - currentYaw);
    float deltaPitch = NormalizeAngleDelta(targetPitch - currentPitch);
    float yawAlpha = LerpExpFactor(m_rotSmoothK, dt);
    float pitchAlpha = LerpExpFactor(m_rotSmoothK, dt);

    float maxYawChange = m_rotateSpeed * dt;
    float maxPitchChange = m_pitchSpeed * dt;
    if (m_isBoosting)
    {
        float boostTurnFactor = 0.5f;
        maxYawChange *= boostTurnFactor;
        maxPitchChange *= boostTurnFactor;
    }

    float appliedYaw = deltaYaw * yawAlpha;
    appliedYaw = std::clamp(appliedYaw, -maxYawChange, maxYawChange);
    currentYaw += appliedYaw;

    float appliedPitch = deltaPitch * pitchAlpha;
    appliedPitch = std::clamp(appliedPitch, -maxPitchChange, maxPitchChange);
    currentPitch += appliedPitch;

    // 新前方ベクトル
    Vector3 newForward = Vector3(
        std::sin(currentYaw) * std::cos(currentPitch),
        std::sin(currentPitch),
        std::cos(currentYaw) * std::cos(currentPitch)
    );
    if (newForward.LengthSquared() > 1e-6f) newForward.Normalize();
    else newForward = Vector3(0, 0, 1);

        // --- 改良版：ロール（銀行）と垂直傾き（ノーズ上下） ---
    // newForward, desired, currentYaw, currentPitch, m_velocity, currentSpeed がここで有効である前提

    // lateral（横成分）は newForward x desired の Y 成分を使う（既に前で計算している場合は重複しても可）
    Vector3 cross = newForward.Cross(desired);
    float lateral = cross.y; // 典型的に -1..1 の範囲に近い

    // 1) ヨー速度計算（前フレームとの差分から）
    float safeDt = max(1e-6f, dt);
    float yawDeltaForRoll = NormalizeAngleDelta(currentYaw - m_prevYaw);
    float yawSpeed = yawDeltaForRoll / safeDt; // rad/s

    // 2) 各成分を重み付けして合成
    float fromYaw = yawSpeed * m_rollYawFactor;          // ヨーからの寄与
    float fromLateral = -lateral * m_rollLateralFactor;  // 横入力からの寄与（符号は好みで反転）
    speedRatio = (m_baseSpeed > 1e-6f) ? (currentSpeed / m_baseSpeed) : 1.0f;
    float speedScale = std::clamp(speedRatio * m_rollSpeedScale, 0.5f, 2.0f);

    float rawRoll = (fromYaw + fromLateral) * speedScale;
    rawRoll = std::clamp(rawRoll, -m_maxVisualRoll, m_maxVisualRoll);

    // 3) 滑らかに補間（指数遅延）
    float rollAlpha = LerpExpFactor(m_rollLerpK, dt);
    m_currentRoll = m_currentRoll + (rawRoll - m_currentRoll) * rollAlpha;

    // 4) 垂直（上下）によるピッチ傾き（ノーズの上下）
    float vertVel = m_velocity.y;

    // 1) ターゲット傾きを非線形で計算してジャンプを和らげる（atan を使用）
    float pitchTarget = -std::atan(vertVel / m_pitchSaturationFactor) * m_verticalTiltFactor;
    // clamp
    pitchTarget = std::clamp(pitchTarget, -m_maxVerticalTilt, m_maxVerticalTilt);

    // 2) スムージング（指数ローパス）で“じわじわ”感を作る
    pitchAlpha = LerpExpFactor(m_pitchTiltSmoothK, dt);
    float newPitch = m_visualPitchTilt + (pitchTarget - m_visualPitchTilt) * pitchAlpha;

    // 3) 変化速度リミットを適用して急激な方向転換を制限する
    const float maxDeltaRad = (m_maxPitchDeltaDegPerSec * (3.14159265358979323846f / 180.0f)) * dt;
    float delta = newPitch - m_visualPitchTilt;
    if (delta > maxDeltaRad) delta = maxDeltaRad;
    if (delta < -maxDeltaRad) delta = -maxDeltaRad;
    m_visualPitchTilt += delta;

    // 5) 回転をセット（元の pitch/yaw を基準に視覚用オフセットを加える）
    rot.x = -currentPitch + m_visualPitchTilt;
    rot.y = currentYaw;
    rot.z = -m_currentRoll; // 視覚ロールを Z 軸に設定（必要なら -m_currentRoll にする）
    GetOwner()->SetRotation(rot);

    // 6) 次フレーム用 yaw 保存
    m_prevYaw = currentYaw;


    // ----- ここから velocity 統合・位置更新 -----
    Vector3 desiredVel = newForward * currentSpeed + m_externalVelocity;

    // velocity を保存 (CollisionResolver / HandleCollisionCorrection が使う)
    m_velocity = desiredVel;

    // integrate position
    pos += m_velocity * dt;

    // PlayArea 補正
    if (m_playArea)
    {
        pos = m_playArea->ResolvePosition(GetOwner()->GetPosition(), pos);
    }
    else
    {
        if (pos.y < -1.0f) pos.y = -1.0f;
    }

    // 最後に SetPosition（CollisionManager はこのあとに呼ばれて押し出し処理を行う構成）
    GetOwner()->SetPosition(pos);

    // 更新保存
    m_currentPitch = currentPitch;
}

// MoveComponent.cpp (追加)
void MoveComponent::HandleCollisionCorrection(const DirectX::SimpleMath::Vector3& push, const DirectX::SimpleMath::Vector3& contactNormal)
{
    using namespace DirectX::SimpleMath;

    GameObject* owner = GetOwner();
    if (!owner) return;

    // 1) 位置を押し出す（CollisionManager が出したワールド単位の push を使う）
    owner->SetPosition(owner->GetPosition() + push);

    // 2) 速度の法線成分を取り除く（内向きの成分のみ）
    if (m_velocity.LengthSquared() > 1e-8f)
    {
        float vn = m_velocity.Dot(contactNormal);
        // vn < 0 -> m_velocity が法線方向に「突っ込む」成分を持つ（内向き）
        if (vn < 0.0f)
        {
            m_velocity = m_velocity - contactNormal * vn;
        }
    }

    // 3) 外部インパルスにも同様に
    if (m_externalVelocity.LengthSquared() > 1e-8f)
    {
        float extn = m_externalVelocity.Dot(contactNormal);
        if (extn < 0.0f)
        {
            m_externalVelocity = m_externalVelocity - contactNormal * extn;
        }
    }

    // 4) 接線成分に摩擦を入れて少し減衰（オプション、調整可）
    Vector3 tangent = m_velocity - contactNormal * m_velocity.Dot(contactNormal);
    m_velocity = tangent * 0.85f; // 0.85 は摩擦係数（好みに合わせて）

    // 5) フラグ：今フレームはコリジョンで補正済み（Update側で利用）
    m_collisionCorrectedThisFrame = true;
}


