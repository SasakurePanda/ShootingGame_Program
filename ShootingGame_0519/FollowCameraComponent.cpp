// FollowCameraComponent.cpp
#define NOMINMAX
#include <cmath> 
#include "FollowCameraComponent.h"
#include "Renderer.h"
#include "Application.h"
#include "Input.h"
#include "PlayAreaComponent.h"
#include <SimpleMath.h>
#include <algorithm>

using namespace DirectX;
using namespace DirectX::SimpleMath;

FollowCameraComponent::FollowCameraComponent()
{
    // スプリングの初期設定（通常時）
    m_normalStiffness = 12.0f;
    m_normalDamping = 6.0f;
    m_Spring.SetStiffness(m_normalStiffness);
    m_Spring.SetDamping(m_normalDamping);
    m_Spring.SetMass(1.0f);

    // プロジェクション
    float width = static_cast<float>(Application::GetWidth());
    float height = static_cast<float>(Application::GetHeight());
    m_ProjectionMatrix = Matrix::CreatePerspectiveFieldOfView(
        XMConvertToRadians(45.0f), width / height, 0.1f, 1000.0f);

    // ブースト関連初期値
    m_boostRequested = false;
    m_boostBlend = 0.0f;
    m_boostBlendSpeed = 6.0f;

	//カメラ振動用の乱数初期化
    //m_shakeRng.seed(123456789ULL);
}

void FollowCameraComponent::SetTarget(GameObject* target)
{
    m_Target = target;
    if (target)
    {
        Vector3 initial = target->GetPosition() + Vector3(0, m_DefaultHeight, -m_DefaultDistance);
        m_Spring.Reset(initial);
        m_PrevPlayerYaw = m_Target->GetRotation().y;
        m_CurrentTurnOffset = 0.0f;
    }
}

void FollowCameraComponent::SetBoostState(bool isBoosting)
{
    // ブースト中かどうかのStateをセット
    m_boostRequested = isBoosting;
}

Vector3 FollowCameraComponent::GetForward() const
{
    return m_ViewMatrix.Invert().Forward();
}

Vector3 FollowCameraComponent::GetRight() const
{
    return m_ViewMatrix.Invert().Right();
}

Vector3 FollowCameraComponent::GetAimDirectionFromReticle() const
{
    if (!m_Target)
    {
        return GetForward();
    }

    Vector3 cameraPos = m_Spring.GetPosition();
    Vector3 targetPos = m_Target->GetPosition();
    Matrix provisionalView = Matrix::CreateLookAt(cameraPos, targetPos, Vector3::Up);

    float screenW = static_cast<float>(Application::GetWidth());
    float screenH = static_cast<float>(Application::GetHeight());
    float sx = m_ReticleScreen.x;
    float sy = m_ReticleScreen.y;

    XMMATRIX projXM = XMLoadFloat4x4(reinterpret_cast<const XMFLOAT4X4*>(&m_ProjectionMatrix));
    XMMATRIX viewXM = XMLoadFloat4x4(reinterpret_cast<const XMFLOAT4X4*>(&provisionalView));
    XMMATRIX worldXM = XMMatrixIdentity();

    XMVECTOR nearScreen = XMVectorSet(sx, sy, 0.0f, 1.0f);
    XMVECTOR farScreen = XMVectorSet(sx, sy, 1.0f, 1.0f);

    XMVECTOR nearWorldV = XMVector3Unproject(
        nearScreen, 0.0f, 0.0f, screenW, screenH, 0.0f, 1.0f, projXM, viewXM, worldXM);
    XMVECTOR farWorldV = XMVector3Unproject(
        farScreen, 0.0f, 0.0f, screenW, screenH, 0.0f, 1.0f, projXM, viewXM, worldXM);

    Vector3 nearWorld(XMVectorGetX(nearWorldV), XMVectorGetY(nearWorldV), XMVectorGetZ(nearWorldV));
    Vector3 farWorld(XMVectorGetX(farWorldV), XMVectorGetY(farWorldV), XMVectorGetZ(farWorldV));

    Vector3 dir = farWorld - nearWorld;
    if (dir.LengthSquared() > 1e-6f)
    {
        dir.Normalize();
    }
    else
    {
        dir = GetForward();
    }

    // --- 垂直スケール適用 ---
    dir.y *= m_VerticalAimScale;
    if (dir.LengthSquared() > 1e-6f)
    {
        dir.Normalize();
    }
    else
    {
        // もしスケールでゼロに近くなったらフォワードを返す
        dir = GetForward();
    }

    return dir;
}

void FollowCameraComponent::Update(float dt)
{
    if (!m_Target)
    {
        return;
    }

    m_IsAiming = Input::IsMouseRightDown();
    POINT delta = Input::GetMouseDelta();
    m_Yaw += delta.x * m_Sensitivity;
    m_Pitch += delta.y * m_Sensitivity;
    m_Pitch = std::clamp(m_Pitch, m_PitchLimitMin, m_PitchLimitMax);
    m_Yaw = std::clamp(m_Yaw, -m_YawLimit, m_YawLimit);

    // ブーストブレンドを目標に向かって滑らかに変化
    float targetBlend;
    if (m_boostRequested)
    {
        targetBlend = 1.0f;
    }
    else
    {
        targetBlend = 0.0f;
    }

    float blendDelta = (targetBlend - m_boostBlend);
    float maxStep = std::min(1.0f, m_boostBlendSpeed * dt);
    if (fabs(blendDelta) <= maxStep)
    {
        m_boostBlend = targetBlend;
    }
    else
    {
        if (blendDelta > 0.0f)
        {
            m_boostBlend += maxStep;
        }
        else
        {
            m_boostBlend -= maxStep;
        }
    }

    // カメラ位置の更新（今回はブーストで距離のみ変動）
    UpdateCameraPosition(dt);

    // カメラと追尾対象のPos取得
    Vector3 cameraPos = m_Spring.GetPosition();
    cameraPos += m_shakeOffset;
    Vector3 targetPos = m_Target->GetPosition();

    float screenW = static_cast<float>(Application::GetWidth());
    float screenH = static_cast<float>(Application::GetHeight());
    float sx = m_ReticleScreen.x;
    float sy = m_ReticleScreen.y;

    Matrix provisionalView = Matrix::CreateLookAt(cameraPos, targetPos, Vector3::Up);
    XMVECTOR nearScreen = XMVectorSet(sx, sy, 0.0f, 1.0f);
    XMVECTOR farScreen = XMVectorSet(sx, sy, 1.0f, 1.0f);

    XMMATRIX projXM = XMLoadFloat4x4(reinterpret_cast<const XMFLOAT4X4*>(&m_ProjectionMatrix));
    XMMATRIX viewXM = XMLoadFloat4x4(reinterpret_cast<const XMFLOAT4X4*>(&provisionalView));
    XMMATRIX worldXM = XMMatrixIdentity();

    XMVECTOR nearWorldV = XMVector3Unproject(
        nearScreen, 0.0f, 0.0f, screenW, screenH, 0.0f, 1.0f, projXM, viewXM, worldXM);
    XMVECTOR farWorldV = XMVector3Unproject(
        farScreen, 0.0f, 0.0f, screenW, screenH, 0.0f, 1.0f, projXM, viewXM, worldXM);

    Vector3 nearWorld(XMVectorGetX(nearWorldV), XMVectorGetY(nearWorldV), XMVectorGetZ(nearWorldV));
    Vector3 farWorld(XMVectorGetX(farWorldV), XMVectorGetY(farWorldV), XMVectorGetZ(farWorldV));

    Vector3 rayDir = farWorld - nearWorld;
    if (rayDir.LengthSquared() > 1e-6f)
    {
        rayDir.Normalize();
    }

    Vector3 camForward = GetForward();

    // ※変更点：ブーストで AimPlaneDistance を変えない（注視計算は常に同じ）
    float localAimPlaneDist = m_AimPlaneDistance;

    Vector3 planePoint = cameraPos + camForward * localAimPlaneDist;
    float denom = rayDir.Dot(camForward);

    Vector3 worldTarget;
    const float EPS = 1e-5f;
    if (fabs(denom) < EPS)
    {
        worldTarget = nearWorld + rayDir * localAimPlaneDist;
    }
    else
    {
        float t = (planePoint - nearWorld).Dot(camForward) / denom;
        worldTarget = nearWorld + rayDir * t;
    }

    const float aimLerp = 12.0f;
    m_AimPoint = m_AimPoint + (worldTarget - m_AimPoint) * std::min(1.0f, aimLerp * dt);

    Vector3 aimDir = m_AimPoint - targetPos;
    if (aimDir.LengthSquared() > 1e-6f)
    {
        aimDir.Normalize();
    }
    else
    {
        aimDir = GetForward();
    }

    // --- 垂直スケールをここで適用してから正規化 ---
    aimDir.y = 0;
    if (aimDir.LengthSquared() > 1e-6f)
    {
        aimDir.Normalize();
    }
    else
    {
        aimDir = GetForward();
    }

    Vector3 rawLookTarget = targetPos + aimDir * m_LookAheadDistance + Vector3(0.0f, (m_DefaultHeight + m_AimHeight) * 0.5f, 0.0f);

    Vector3 targetRot = m_Target->GetRotation();
    Matrix  playerRot = Matrix::CreateRotationY(targetRot.y);
    Vector3 localRight = Vector3::Transform(Vector3::Right, playerRot);

    // ※変更点：lookOffsetMul はブーストに依存させない（常に 0.6）
    float lookOffsetMul = 0.6f;
    rawLookTarget += localRight * (m_CurrentTurnOffset * lookOffsetMul);

    m_LookTarget = m_LookTarget + (rawLookTarget - m_LookTarget) * std::min(1.0f, m_LookAheadLerp * dt);

    m_ViewMatrix = Matrix::CreateLookAt(cameraPos, m_LookTarget, Vector3::Up);

    Renderer::SetViewMatrix(m_ViewMatrix);
    Renderer::SetProjectionMatrix(m_ProjectionMatrix);
}

void FollowCameraComponent::UpdateCameraPosition(float dt)
{
    if (!m_Target) return;

    // 1) 基本距離・高さ決定（ブースト要求がある時だけ距離を伸ばす）
    float desiredDist;
    float height;
    if (m_IsAiming) {
        desiredDist = m_AimDistance;
        height = m_AimHeight;
    }
    else {
        desiredDist = m_DefaultDistance;
        height = m_DefaultHeight;
    }
    if (m_boostRequested) {
        desiredDist += m_boostAimDistanceAdd; // ブースト時のみ距離を伸ばす
    }

    // 2) プレイヤー情報取得
    Vector3 targetPos = m_Target->GetPosition();
    Vector3 targetRot = m_Target->GetRotation();
    float playerYaw = targetRot.y;
    Matrix playerRot = Matrix::CreateRotationY(playerYaw);

    // 3) baseDesired を作る（この時点でプレイヤーからの距離 = desiredDist を満たす）
    Vector3 baseOffset = Vector3(0.0f, 0.0f, -desiredDist);
    Vector3 rotatedOffset = Vector3::Transform(baseOffset, playerRot);
    Vector3 baseDesired = targetPos + rotatedOffset + Vector3(0.0f, height, 0.0f);

    // 4) lateral (reticle) および turn offset を計算して baseDesired に加える
    float screenW = static_cast<float>(Application::GetWidth());
    float normX = 0.0f;
    if (screenW > 1.0f) normX = (m_ReticleScreen.x - (screenW * 0.5f)) / (screenW * 0.5f);

    Vector3 localRight = Vector3::Transform(Vector3::Right, playerRot);
    float lateral = normX * m_ScreenOffsetScale;
    lateral = std::clamp(lateral, -m_MaxScreenOffset, m_MaxScreenOffset);

    float yawDelta = playerYaw - m_PrevPlayerYaw;
    while (yawDelta > XM_PI) yawDelta -= XM_2PI;
    while (yawDelta < -XM_PI) yawDelta += XM_2PI;
    float safeDt = std::max(1e-6f, dt);
    float yawSpeed = yawDelta / safeDt;
    float boostScale = 1.0f;
    float turnLateral = yawSpeed * m_TurnOffsetScale * boostScale;
    float turnMax = m_TurnOffsetMax * boostScale;
    turnLateral = std::clamp(turnLateral, -turnMax, turnMax);
    m_CurrentTurnOffset = m_CurrentTurnOffset + (turnLateral - m_CurrentTurnOffset) * std::min(1.0f, m_TurnOffsetLerp * dt);

    // totalDesired: base + lateral + turnOffset
    Vector3 totalDesired = baseDesired;
    totalDesired += localRight * lateral;
    totalDesired += localRight * m_CurrentTurnOffset;

    // 5) PlayArea による Y クランプ（高さはここで決定）
    // まず距離に基づく半高さを計算（今回のカメラからターゲットまでの距離は desiredDist）
    // だがスプリングや laterals により多少ずれる可能性があるため、ここは conservative な計算
    Vector3 camToTargetForHalfHeight = targetPos - totalDesired;
    float distAlongForward = camToTargetForHalfHeight.Length();
    if (distAlongForward < 1e-6f) distAlongForward = 1e-6f;
    float halfHeight = std::tan(m_Fov * 0.5f) * distAlongForward;

    if (m_playArea)
    {
        float boundsMinY = m_playArea->GetBoundsMin().y;
        float boundsMaxY = m_playArea->GetBoundsMax().y;
        float minCameraY = boundsMinY + halfHeight;
        float maxCameraY = boundsMaxY - halfHeight;
        if (minCameraY > maxCameraY) {
            float mid = (minCameraY + maxCameraY) * 0.5f;
            minCameraY = maxCameraY = mid;
        }
        totalDesired.y = std::clamp(totalDesired.y, minCameraY, maxCameraY);
    }

    // 6) 非ブースト時は「高さを保持したまま」XZ（水平）を再スケールして
    //    3D 距離が desiredDist になるように強制する（横オフセットを見せつつ距離は固定）
    if (!m_boostRequested)
    {
        Vector3 toCam = totalDesired - targetPos;
        float heightDiff = toCam.y;
        float distSq = desiredDist * desiredDist;
        float h2 = heightDiff * heightDiff;
        float desiredHoriz = 0.0f;
        if (distSq > h2 + 1e-6f) desiredHoriz = std::sqrt(distSq - h2);
        else desiredHoriz = 0.0f;

        Vector3 horizVec = Vector3(toCam.x, 0.0f, toCam.z);
        float horizLen = horizVec.Length();
        if (horizLen > 1e-6f)
        {
            float scale = desiredHoriz / horizLen;
            totalDesired.x = targetPos.x + horizVec.x * scale;
            totalDesired.z = targetPos.z + horizVec.z * scale;
            // totalDesired.y は既に PlayArea でクランプ済み
        }
        else
        {
            Vector3 forward = Vector3::Transform(Vector3::Forward, Matrix::CreateRotationY(targetRot.y));
            if (forward.LengthSquared() > 1e-6f) forward.Normalize();
            totalDesired.x = targetPos.x + forward.x * desiredHoriz;
            totalDesired.z = targetPos.z + forward.z * desiredHoriz;
        }
    }

    // 7) safety: スプリング位置が極端に遠ければリセット（初期化不足や過去のバグで巨大値が出るケース対策）
    Vector3 springPos = m_Spring.GetPosition();
    const float ABSOLUTE_POS_LIMIT = 10000.0f; // 必要なら調整
    if (std::isnan(springPos.x) || std::isnan(springPos.y) || std::isnan(springPos.z) ||
        springPos.Length() > ABSOLUTE_POS_LIMIT)
    {
        // 初期化（瞬間的にカメラを desired に合わせる）
        m_Spring.Reset(totalDesired);
        springPos = m_Spring.GetPosition();
    }

    // 8) optional: スプリング差が大きければ一時的に stiffness を上げて追従を速める
    Vector3 desiredPos = totalDesired;
    float springDiff = (desiredPos - springPos).Length();
    // パラメータ: このしきい値を越えたら追従を速める（非ブースト時のみ）
    const float DIFF_SNAP_THRESHOLD = desiredDist * 0.5f; // 調整可
    if (!m_boostRequested && springDiff > DIFF_SNAP_THRESHOLD)
    {
        // 一時的に stiffness を上げる（m_Spring に SetStiffness があるものとする）
        m_Spring.SetStiffness(m_normalStiffness * 1.8f);
        m_Spring.SetDamping(m_normalDamping * 1.0f); // 任意調整
    }
    else
    {
        m_Spring.SetStiffness(m_normalStiffness);
        m_Spring.SetDamping(m_normalDamping);
    }

    // 9) 最後にスプリング更新
    m_Spring.Update(desiredPos, dt);

    // 10) prev yaw 更新
    m_PrevPlayerYaw = playerYaw;

    // 11) 振動処理（既存ロジックを保持）
    m_shakeOffset = Vector3::Zero;
    if (m_shakeTimeRemaining > 0.0f && m_Target)
    {
        float t = m_shakeTimeRemaining / std::max(1e-6f, m_shakeTotalDuration);
        float falloff = t;
        m_shakePhase += dt * m_shakeFrequency * 2.0f * XM_PI;
        float sampleX = std::sin(m_shakePhase);
        float sampleY = std::sin(m_shakePhase * 1.5f + 3.1f);
        float baseAmp = m_shakeMagnitude * falloff;

        Vector3 cameraPosTemp = m_Spring.GetPosition();
        Vector3 forward = m_Target->GetPosition() - cameraPosTemp;
        if (forward.LengthSquared() > 1e-6f)
        {
            forward.Normalize();
            XMVECTOR vForward = XMLoadFloat3(reinterpret_cast<const XMFLOAT3*>(&forward));
            XMVECTOR vUp = XMLoadFloat3(reinterpret_cast<const XMFLOAT3*>(&Vector3::Up));
            XMVECTOR vRight = XMVector3Cross(vUp, vForward);
            Vector3 camRight; XMStoreFloat3(reinterpret_cast<XMFLOAT3*>(&camRight), vRight);
            if (camRight.LengthSquared() > 1e-6f)
            {
                camRight.Normalize();
                Vector3 camUp = forward.Cross(camRight);
                if (camUp.LengthSquared() > 1e-6f) camUp.Normalize();

                switch (m_shakeMode)
                {
                case ShakeMode::Horizontal:
                    m_shakeOffset = camRight * (baseAmp * (0.7f * sampleX + 0.3f * sampleY));
                    break;
                case ShakeMode::Vertical:
                    m_shakeOffset = camUp * (baseAmp * (0.7f * sampleY + 0.3f * sampleX));
                    break;
                case ShakeMode::ALL:
                    m_shakeOffset = camRight * (baseAmp * (0.7f * sampleX + 0.3f * sampleY))
                        + camUp * (baseAmp * (0.7f * sampleY + 0.3f * sampleX));
                    break;
                }
            }
        }
        m_shakeTimeRemaining -= dt;
        if (m_shakeTimeRemaining <= 0.0f)
        {
            m_shakeMagnitude = 0.0f;
            m_shakeTimeRemaining = 0.0f;
            m_shakeTotalDuration = 0.0f;
            m_shakePhase = 0.0f;
            m_shakeOffset = Vector3::Zero;
        }
    }
}


void FollowCameraComponent::Shake(float magnitude, float duration , ShakeMode mode)
{
    if (magnitude <= 0.0f || duration <= 0.0f) { return; }

	//今までの振動よりも値が大きかったり、振動時間が前よりも長い場合は上書き
    m_shakeMagnitude     = std::max(m_shakeMagnitude, magnitude);
    m_shakeTimeRemaining = std::max(m_shakeTimeRemaining, duration);
    m_shakeTotalDuration = std::max(m_shakeTotalDuration, duration);

    //モードを設定する
	m_shakeMode = mode;

    //フェーズはリセット
    m_shakePhase = 0.0f;
}
