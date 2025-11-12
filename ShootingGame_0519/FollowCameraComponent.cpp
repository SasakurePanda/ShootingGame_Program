// FollowCameraComponent.cpp
#define NOMINMAX
#include "FollowCameraComponent.h"
#include "Renderer.h"
#include "Application.h"
#include "Input.h"
#include <SimpleMath.h>
#include <algorithm>

using namespace DirectX;
using namespace DirectX::SimpleMath;

FollowCameraComponent::FollowCameraComponent()
{
    // スプリングの初期設定（通常時）
    m_normalStiffness = 12.0f;
    m_normalDamping   = 6.0f;
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
    //ブースト中かどうかのStateをセット
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
    if (!m_Target) { return GetForward(); }

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
    if (dir.LengthSquared() > 1e-6f) dir.Normalize();
    else dir = GetForward();

    return dir;
}

void FollowCameraComponent::Update(float dt)
{
    if (!m_Target) { return; }

    m_IsAiming = Input::IsMouseRightDown();
    POINT delta = Input::GetMouseDelta();
    m_Yaw += delta.x * m_Sensitivity;
    m_Pitch += delta.y * m_Sensitivity;
    m_Pitch = std::clamp(m_Pitch, m_PitchLimitMin, m_PitchLimitMax);
    m_Yaw = std::clamp(m_Yaw, -m_YawLimit, m_YawLimit);

    //ブーストブレンドを目標に向かって滑らかに変化
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

    // カメラ位置の更新（内部でブーストにより stiffness/damping 等を補間）
    UpdateCameraPosition(dt);

    //カメラと追尾対象のPos取得
    Vector3 cameraPos = m_Spring.GetPosition();
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

    // ブースト時は 注視平面距離 を増やす（カメラが後ろに残る感じ）
    float localAimPlaneDist = m_AimPlaneDistance + (m_boostBlend * m_boostAimDistanceAdd);

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

    Vector3 rawLookTarget = targetPos + aimDir * m_LookAheadDistance + Vector3(0.0f, (m_DefaultHeight + m_AimHeight) * 0.5f, 0.0f);

    Vector3 targetRot = m_Target->GetRotation();
    Matrix playerRot = Matrix::CreateRotationY(targetRot.y);
    Vector3 localRight = Vector3::Transform(Vector3::Right, playerRot);

    float lookOffsetMul = 0.6f + 0.8f * m_boostBlend;
    rawLookTarget += localRight * (m_CurrentTurnOffset * lookOffsetMul);

    m_LookTarget = m_LookTarget + (rawLookTarget - m_LookTarget) * std::min(1.0f, m_LookAheadLerp * dt);

    m_ViewMatrix = Matrix::CreateLookAt(cameraPos, m_LookTarget, Vector3::Up);

    Renderer::SetViewMatrix(m_ViewMatrix);
    Renderer::SetProjectionMatrix(m_ProjectionMatrix);
}

void FollowCameraComponent::UpdateCameraPosition(float dt)
{
    if (!m_Target) { return; }

    // ブースト時/通常時の spring パラメータを m_boostBlend により補間
    float stiffness = (1.0f - m_boostBlend) * m_normalStiffness + m_boostBlend * m_boostedStiffness;
    float damping = (1.0f - m_boostBlend) * m_normalDamping + m_boostBlend * m_boostedDamping;

    m_Spring.SetStiffness(stiffness);
    m_Spring.SetDamping(damping);

    float dist;
    float height;

    if (m_IsAiming)
    {
        dist = m_AimDistance;
        height = m_AimHeight;
    }
    else
    {
        dist = m_DefaultDistance;
        height = m_DefaultHeight;
    }

    // ブースト時は目標距離を少し伸ばす（カメラが後ろに残る印象）
    dist += m_boostBlend * m_boostAimDistanceAdd;

    Vector3 targetPos = m_Target->GetPosition();
    Vector3 targetRot = m_Target->GetRotation();
    float playerYaw = targetRot.y;

    Matrix playerRot = Matrix::CreateRotationY(playerYaw);
    Vector3 baseOffset = Vector3(0.0f, 0.0f, -dist);
    Vector3 rotatedOffset = Vector3::Transform(baseOffset, playerRot);

    Vector3 desiredPos = targetPos + rotatedOffset + Vector3(0.0f, height, 0.0f);

    float screenW = static_cast<float>(Application::GetWidth());
    float normX = 0.0f;
    if (screenW > 1.0f)
    {
        normX = (m_ReticleScreen.x - (screenW * 0.5f)) / (screenW * 0.5f);
    }
    Vector3 localRight = Vector3::Transform(Vector3::Right, playerRot);
    float lateral = normX * m_ScreenOffsetScale;
    lateral = std::clamp(lateral, -m_MaxScreenOffset, m_MaxScreenOffset);
    desiredPos += localRight * lateral;

    float yawDelta = playerYaw - m_PrevPlayerYaw;
    while (yawDelta > XM_PI) 
    {
        yawDelta -= XM_2PI; 
    }
    while (yawDelta < -XM_PI)
    {
        yawDelta += XM_2PI;
    }

    float safeDt = std::max(1e-6f, dt);
    float yawSpeed = yawDelta / safeDt;

    // ブースト時により大きな横オフセットを与える（距離が伸びると見効果が薄れるため）
    float boostScale = 1.0f + m_boostBlend * 1.5f; // 通常1.0、ブーストで最大 2.5 倍程度
    float turnLateral = yawSpeed * m_TurnOffsetScale * boostScale;
    float turnMax = m_TurnOffsetMax * boostScale;
    turnLateral = std::clamp(turnLateral, -turnMax, turnMax);

    m_CurrentTurnOffset = m_CurrentTurnOffset + (turnLateral - m_CurrentTurnOffset) * std::min(1.0f, m_TurnOffsetLerp * dt);

    // カメラ本体の横移動（プレイヤーの向きに応じてカメラを横にずらす）
    desiredPos += localRight * m_CurrentTurnOffset;

    // --- 重要: 毎フレームの yaw を保存して次フレームの差分に備える ---
    m_PrevPlayerYaw = playerYaw;

    m_Spring.Update(desiredPos, dt);
}
