#include "MoveComponent.h"
#include "PlayAreaComponent.h"
#include "GameObject.h"
#include "Application.h"
#include <iostream>
#include <cmath>
#include <algorithm>
#include <DirectXMath.h>

using namespace DirectX;
using namespace DirectX::SimpleMath;

static float LerpExpFactor(float k, float dt)
{
    return 1.0f - std::exp(-k * dt);
}

static float NormalizeAngleDelta(float a)
{
    while (a > XM_PI)
    {
        a -= XM_2PI;
    }
    while (a < -XM_PI)
    {
        a += XM_2PI;
    }
    return a;
}

static float ApplyDeadZoneAndCurve(float delta, float deadZone, float softZone)
{
    float sign = (delta >= 0.0f) ? 1.0f : -1.0f;
    float mag = std::fabs(delta);

    if (mag <= deadZone)
    {
        return 0.0f;
    }

    float denom = max(softZone - deadZone, 1e-6f);
    float t = (mag - deadZone) / denom;
    t = std::clamp(t, 0.0f, 1.0f);

    float curve = t * t;
    float curvedMag = mag * curve;

    return sign * curvedMag;
}


void MoveComponent::Initialize()
{
    if (GetOwner())
    {
        //前フレームのyawを保存
        m_prevYaw = GetOwner()->GetRotation().y;
    }
    else
    {
        m_prevYaw = 0.0f;
    }

    m_visualPitchTilt = 0.0f;
}

void MoveComponent::Uninit()
{

}

float MoveComponent::GetBoostIntensity() const
{
    if (m_isBoosting)
    {
        float t = m_boostTimer / m_boostSeconds;
        return std::clamp(t, 0.0f, 1.0f);
    }
    else if (m_recoverTimer >= 0.0f)
    {
        float t = 1.0f - std::clamp(m_recoverTimer / m_boostRecover, 0.0f, 1.0f);
        return t;
    }
    else
    {
        return 0.0f;
    }
}

void MoveComponent::Update(float dt)
{
    using namespace DirectX::SimpleMath;

    //カメラ or オーナーがなければ何もしない
    if (!m_camera || !GetOwner()) { return; }
    if (dt <= 0.0f) { return; }

    GameObject* owner = GetOwner();

    //現在位置・回転取得
    Vector3 pos = owner->GetPosition();
    Vector3 rot = owner->GetRotation();

    float currentYaw = rot.y;
    float currentPitch = m_currentPitch;

    //--------------------ブースト入力処理---------------------
    bool keyDown = Input::IsKeyPressed(m_boostKey);

    bool startBoost = false;

    //キーが押されている&クールダウンが終わっている&前フレームでキーが押されていないか
    if (keyDown && !m_prevBoostKeyDown && m_cooldownTimer <= 0.0f && !m_isBoosting)
    {
        startBoost = true;
    }
    m_prevBoostKeyDown = keyDown;

    //ブーストがスタートしたなら
    if (startBoost)
    {
        m_isBoosting = true;    //ブースト開始
        m_boostTimer = 0.0f;    //経過時間リセット
        m_recoverTimer = -1.0f;
        m_cooldownTimer = m_boostCooldown;  //
        //カメラの動きにも関係があるので送る
        if (m_camera)
        {
            m_camera->SetBoostState(true);
        }
    }

    //ブースト中なら
    if (m_isBoosting)
    {
        m_boostTimer += dt;

        //ブーストが終わったら
        if (m_boostTimer >= m_boostSeconds)
        {
            m_isBoosting = false;
            m_recoverTimer = 0.0f;
            //カメラの動きにも関係があるので送る
            if (m_camera)
            {
                m_camera->SetBoostState(false);
            }
        }
    }

    //クールタイムが0秒より大きいなら
    if (m_cooldownTimer > 0.0f)
    {
        m_cooldownTimer -= dt;
        if (m_cooldownTimer < 0.0f)
        {
            m_cooldownTimer = 0.0f;
        }
    }

    //-------------------------スピード決定-------------------------

    float currentSpeed = m_baseSpeed;

    //ブースト中なら
    if (m_isBoosting)
    {
        //元の速度×ブーストの速度倍率
        currentSpeed = m_baseSpeed * m_boostMultiplier;
    }
    //回復時間が0秒以上&回復時間にまだ達していない
    else if (m_recoverTimer >= 0.0f && m_recoverTimer < m_boostRecover)
    {
        m_recoverTimer += dt;
        //回復経過時間÷回復の時間を0~1に収める
        float t = std::clamp(m_recoverTimer / m_boostRecover, 0.0f, 1.0f);
        //イージングして、勢いよく戻らないように
        float ease = 1.0f - (1.0f - t) * (1.0f - t);
        float currentMultiplier = 1.0f + (m_boostMultiplier - 1.0f) * (1.0f - ease);
        currentSpeed = m_baseSpeed * currentMultiplier;
    }

    //--------------------レティクル方向の計算--------------------

    //カメラ側でレティクルのワールド座標を返す
    Vector3 aimTarget = m_camera->GetAimPoint();

    //プレイヤー位置からレティクルのワールド座標へのベクトル
    Vector3 toTarget = aimTarget - pos;

    if (toTarget.LengthSquared() < 1e-6f)
    {
        //目標点がほぼ現在位置なら、今の向きで進むだけ
        Vector3 forward(std::sin(currentYaw) * std::cos(currentPitch),
            std::sin(currentPitch),
            std::cos(currentYaw) * std::cos(currentPitch));
        forward.Normalize();

        //移動速度ベクトル = 前方向ベクトル * 今の移動スピード + 外部からかかる力 
        m_velocity = forward * currentSpeed + m_externalVelocity;

        //位置 = 移動速度ベクトル * デルタタイム
        pos += m_velocity * dt;

        //プレイエリアでの範囲制限
        if (m_playArea)
        {
            //エリアの範囲に収める処理
            pos = m_playArea->ResolvePosition(owner->GetPosition(), pos);
        }
        else
        {
            //最低限下にはいかないようにする
            if (pos.y < -1.0f)
            {
                pos.y = -1.0f;
            }
        }

        owner->SetPosition(pos);
        m_currentPitch = currentPitch;
        return;
    }

    Vector3 desired = toTarget;
    desired.Normalize();

    // 現在の yaw だけから「前方」と「右方向」の基準ベクトルを作る
    Vector3 forwardYaw(
        std::sin(currentYaw),
        0.0f,
        std::cos(currentYaw)
    );
    if (forwardYaw.LengthSquared() > 1e-6f)
    {
        forwardYaw.Normalize();
    }
    else
    {
        forwardYaw = Vector3(0, 0, 1);
    }

    Vector3 rightYaw = Vector3::Up.Cross(forwardYaw);   // ← メンバー関数で呼ぶ
    if (rightYaw.LengthSquared() > 1e-6f)
    {
        rightYaw.Normalize();
    }

    // desired を「前方向・右方向・上下」に分解
    float f = desired.Dot(forwardYaw); // 前後成分
    float r = desired.Dot(rightYaw);   // 左右成分
    float u = desired.y;               // 上下成分

    // ★ 左右成分だけ弱める（0.0～1.0 で調整）
    const float lateralScale = 0.4f;   // 小さいほど左右への曲がりが弱くなる
    r *= lateralScale;

    // 弱めた左右成分で方向ベクトルを再構成
    Vector3 blended = forwardYaw * f + rightYaw * r + Vector3(0.0f, u, 0.0f);
    if (blended.LengthSquared() > 1e-6f)
    {
        blended.Normalize();
        desired = blended;
    }

    if (desired.LengthSquared() < 1e-6f)
    {
        //保険
        desired = Vector3(0, 0, 1);
    }

    //------------------------目標ヨー/ピッチを計算-----------------------
    //XZ平面上で前方向(z軸)にどれだけ回っているか(左右)
    float targetYaw = std::atan2(desired.x, desired.z);
    float horiz = std::sqrt(desired.x * desired.x + desired.z * desired.z);
    float targetPitch = std::atan2(desired.y, horiz);

    float deltaYaw = NormalizeAngleDelta(targetYaw - currentYaw);
    float deltaPitch = NormalizeAngleDelta(targetPitch - currentPitch);

    const float yawDeadZone = DirectX::XMConvertToRadians(3.0f);
    const float yawSoftZone = DirectX::XMConvertToRadians(13.0f);
    const float pitchDeadZone = DirectX::XMConvertToRadians(2.0f);
    const float pitchSoftZone = DirectX::XMConvertToRadians(15.0f);

    deltaYaw   = ApplyDeadZoneAndCurve(deltaYaw, yawDeadZone, yawSoftZone);
    deltaPitch = ApplyDeadZoneAndCurve(deltaPitch, pitchDeadZone, pitchSoftZone);

    //------------------ヨー/ピッチをスムーズに追従させる------------------
    //ヨーの指数補間係数でどんな感じで動くか決める
    float yawAlpha = LerpExpFactor(m_rotSmoothK, dt);

    //ピッチは少しだけ重めにしてヌルっとさせる
    const float pitchSmoothK = m_rotSmoothK * 0.6f;

    //ピッチの指数補間係数でどんな感じで動くか決める
    float pitchAlpha = LerpExpFactor(pitchSmoothK, dt);

    //回転のマックス量設定
    float maxYawChange = m_rotateSpeed * dt;
    float maxPitchChange = m_pitchSpeed * dt;

    //ブースト中なら
    if (m_isBoosting)
    {
        //あんまり勢いよく回転しないようにする
        float boostTurnFactor = 0.5f;
        maxYawChange *= boostTurnFactor;
        maxPitchChange *= boostTurnFactor;
    }

    float appliedYaw = std::clamp(deltaYaw * yawAlpha, -maxYawChange, maxYawChange);
    currentYaw += appliedYaw;

    float appliedPitch = std::clamp(deltaPitch * pitchAlpha, -maxPitchChange, maxPitchChange);
    currentPitch += appliedPitch;

    //----------------------新しい前方ベクトル----------------------
    Vector3 newForward(std::sin(currentYaw) * std::cos(currentPitch),
        std::sin(currentPitch),
        std::cos(currentYaw) * std::cos(currentPitch));

    if (newForward.LengthSquared() > 1e-6f)
    {
        //正規化
        newForward.Normalize();
    }
    else
    {
        //保険
        newForward = Vector3(0, 0, 1);
    }

    //--------------------------ロール演出--------------------------
    // 目標向き(desired)と現在の前方(newForward)のクロスで「横方向のずれ」を取る
    Vector3 cross = newForward.Cross(desired);
    float   lateral = cross.y;

    float safeDt = max(1e-6f, dt);
    float yawDeltaForRoll = NormalizeAngleDelta(currentYaw - m_prevYaw);
    float yawSpeed = yawDeltaForRoll / safeDt; // ヨー回転の速さ(ラジアン/秒)

    // 「ほぼまっすぐ」の判定用しきい値（必要に応じて調整）
    const float lateralDeadZone = 0.05f; // 横方向のずれがこの範囲内なら無視
    const float yawDeadZoneRoll = 0.25f; // ヨー回転速度がこの範囲内なら無視

    // ★カーソルを動かしていない＝ほぼ直進 → ロールを0に戻すだけ★
    if (std::fabs(lateral) < lateralDeadZone &&
        std::fabs(yawSpeed) < yawDeadZoneRoll)
    {
        // 目標ロール 0 に向かって指数的に戻す
        float rollAlpha = LerpExpFactor(m_rollLerpK, dt);
        m_currentRoll = m_currentRoll + (0.0f - m_currentRoll) * rollAlpha;
    }
    else
    {
        // ここは「曲がっている最中」のロール演出
        float speedRatio = (m_baseSpeed > 1e-6f) ? (currentSpeed / m_baseSpeed) : 1.0f;
        float fromYaw = yawSpeed * m_rollYawFactor;
        float fromLateral = -lateral * m_rollLateralFactor;
        float speedScale = std::clamp(speedRatio * m_rollSpeedScale, 0.5f, 1.5f);

        float rawRoll = (fromYaw + fromLateral) * speedScale;

        // 全体のロールの振れ幅を制限
        rawRoll = std::clamp(rawRoll, -m_maxVisualRoll, m_maxVisualRoll);

        float rollAlpha = LerpExpFactor(m_rollLerpK, dt);
        m_currentRoll = m_currentRoll + (rawRoll - m_currentRoll) * rollAlpha;
    }

    //================= 8. 見た目用の縦揺れ（いらなければ全部0にしてもOK） =================
    float vertComponent = newForward.y * currentSpeed;
    float pitchTargetTilt =
        -std::atan(vertComponent / m_pitchSaturationFactor) * m_verticalTiltFactor;
    pitchTargetTilt = std::clamp(pitchTargetTilt, -m_maxVerticalTilt, m_maxVerticalTilt);

    float pitchTiltAlpha = LerpExpFactor(m_pitchTiltSmoothK, dt);
    float newVisualPitch =
        m_visualPitchTilt + (pitchTargetTilt - m_visualPitchTilt) * pitchTiltAlpha;

    const float maxDeltaRad =
        (m_maxPitchDeltaDegPerSec * (3.14159265358979323846f / 180.0f)) * dt;
    float delta = newVisualPitch - m_visualPitchTilt;
    if (delta > maxDeltaRad) delta = maxDeltaRad;
    if (delta < -maxDeltaRad) delta = -maxDeltaRad;
    m_visualPitchTilt += delta;

    //================= 9. 回転をオブジェクトに反映 =================
    rot.x = -currentPitch + m_visualPitchTilt; // 上下 = ピッチ + 演出
    rot.y = currentYaw;
    rot.z = -m_currentRoll;
    owner->SetRotation(rot);
    m_prevYaw = currentYaw;

    //================= 10. 速度 & 位置更新 =================
    m_velocity = newForward * currentSpeed + m_externalVelocity;

    pos += m_velocity * dt;

    if (m_playArea)
    {
        pos = m_playArea->ResolvePosition(owner->GetPosition(), pos);
    }
    else
    {
        if (pos.y < -1.0f)
        {
            pos.y = -1.0f;
        }
    }

    owner->SetPosition(pos);

    if (m_externalVelocity.LengthSquared() > 1e-8f)
    {
        float decay = std::exp(-m_externalDamping * dt);
        m_externalVelocity *= decay;

        // 微小な値はクリアしておく
        if (m_externalVelocity.LengthSquared() < 1e-6f)
        {
            m_externalVelocity = DirectX::SimpleMath::Vector3::Zero;
        }
    }

    m_currentPitch = currentPitch;


}
void MoveComponent::HandleCollisionCorrection(
    const DirectX::SimpleMath::Vector3& push,
    const DirectX::SimpleMath::Vector3& contactNormal)
{
    using namespace DirectX::SimpleMath;

    GameObject* owner = GetOwner();
    if (!owner) { return; }

    // 押し出しがほぼゼロなら無視
    if (push.LengthSquared() < 1e-8f){ return; }

    // --- 押し出し方向ベクトル（法線）を決める ---
    Vector3 n;

    if (contactNormal.LengthSquared() > 1e-8f)
    {
        n = contactNormal;
        n.Normalize();
    }
    else
    {
        // contactNormal が変なら push 方向から作る
        Vector3 tmp = push;
        if (tmp.LengthSquared() > 1e-8f)
        {
            tmp.Normalize();
        }
        else
        {

            tmp = Vector3::Up;
        }
        n = tmp;
    }

    //位置補正
    m_totalPushThisFrame += push;
    m_hasPushThisFrame = true;

    //壁の内側に向かう速度成分を削る
    auto KillInwardComponent = [&](Vector3& v)
        {
            if (v.LengthSquared() < 1e-8f) return;

            float vn = v.Dot(n);
            // vn < 0 → 法線の逆向き（中に向かっている）
            if (vn < 0.0f)
            {
                v = v - n * vn;
            }
        };

    KillInwardComponent(m_velocity);
    KillInwardComponent(m_externalVelocity);
}

void MoveComponent::ApplyCollisionPush()
{
    using namespace DirectX::SimpleMath;

    if (!m_hasPushThisFrame || !GetOwner())
    {
        // 何もなければリセットだけ
        m_totalPushThisFrame = Vector3::Zero;
        m_hasPushThisFrame = false;
        return;
    }

    // 1フレーム分まとめて位置を押し出す
    Vector3 pos = GetOwner()->GetPosition();
    pos += m_totalPushThisFrame;
    GetOwner()->SetPosition(pos);

    // 次フレームに持ち越さない
    m_totalPushThisFrame = Vector3::Zero;
    m_hasPushThisFrame = false;
}

void MoveComponent::RequestBoost()
{
    if (m_isBoosting || m_cooldownTimer > 0.0f)
    {
        return;
    }

    m_isBoosting = true;
    m_boostTimer = 0.0f;
    m_recoverTimer = -1.0f;
    m_cooldownTimer = m_boostCooldown;

    if (m_camera)
    {
        m_camera->SetBoostState(true);
    }
}