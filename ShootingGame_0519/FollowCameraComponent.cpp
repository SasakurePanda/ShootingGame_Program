#include <DirectXMath.h>
#include "FollowCameraComponent.h"
#include "Renderer.h"
#include "Application.h"
#include "Input.h"

using namespace DirectX;

FollowCameraComponent::FollowCameraComponent()
{
    // スプリング設定
    m_Spring.SetStiffness(12.0f);
    m_Spring.SetDamping(6.0f);
    m_Spring.SetMass(1.0f);

    // プロジェクション行列
    float width = static_cast<float>(Application::GetWidth());
    float height = static_cast<float>(Application::GetHeight());
    m_ProjectionMatrix = Matrix::CreatePerspectiveFieldOfView(
        XMConvertToRadians(45.0f), width / height, 0.1f, 1000.0f);
}

//カメラが追う対象をセットする関数
void FollowCameraComponent::SetTarget(GameObject* target)
{
    m_Target = target;
    if (target)
    {
        //ターゲット位置 + (高さ, 後方) のオフセットで初期カメラ位置を決める
        Vector3 initial = target->GetPosition() + Vector3(0, m_DefaultHeight, -m_DefaultDistance);
        m_Spring.Reset(initial);
    }
}

//
void FollowCameraComponent::Update(float dt)
{
    //ターゲットがいないなら処理しない
    if (!m_Target) return;

    //エイム（右クリックのみ）
    m_IsAiming = Input::IsMouseRightDown();

    //マウスによる視点制御
    POINT delta = Input::GetMouseDelta();
    m_Yaw += delta.x * m_Sensitivity;
    m_Pitch += delta.y * m_Sensitivity;

    //角度制限（ラジアン）
    m_Pitch = std::clamp(m_Pitch, m_PitchLimitMin, m_PitchLimitMax);
    m_Yaw = std::clamp(m_Yaw, -m_YawLimit, m_YawLimit);

    //カメラ位置（プレイヤーの背後）をスプリングに与える
    //UpdateCameraPosition() 内で m_Spring.Update(desiredPos, dt) を呼ぶようにしてください
    UpdateCameraPosition();

    //カメラの現在位置（スプリングから取得）
    Vector3 cameraPos = m_Spring.GetPosition();
    Vector3 targetPos = m_Target->GetPosition();

    //Unproject を使ってレティクルのワールド点を計算
    float screenW = static_cast<float>(Application::GetWidth());
    float screenH = static_cast<float>(Application::GetHeight());
    float sx = m_ReticleScreen.x;
    float sy = m_ReticleScreen.y;

    Matrix provisionalView = Matrix::CreateLookAt(cameraPos, targetPos, Vector3::Up);

    XMVECTOR nearScreen = XMVectorSet(sx, sy, 0.0f, 1.0f);
    XMVECTOR farScreen = XMVectorSet(sx, sy, 1.0f, 1.0f);

    // Convert SimpleMath::Matrix to XMMATRIX
    XMMATRIX projXM = XMLoadFloat4x4(reinterpret_cast<const XMFLOAT4X4*>(&m_ProjectionMatrix));
    XMMATRIX viewXM = XMLoadFloat4x4(reinterpret_cast<const XMFLOAT4X4*>(&provisionalView));
    XMMATRIX worldXM = XMMatrixIdentity();

    XMVECTOR nearWorldV = XMVector3Unproject(
        nearScreen,
        0.0f, 0.0f, screenW, screenH,
        0.0f, 1.0f,
        projXM,
        viewXM,
        worldXM);

    XMVECTOR farWorldV = XMVector3Unproject(
        farScreen,
        0.0f, 0.0f, screenW, screenH,
        0.0f, 1.0f,
        projXM,
        viewXM,
        worldXM);

    Vector3 nearWorld(XMVectorGetX(nearWorldV), XMVectorGetY(nearWorldV), XMVectorGetZ(nearWorldV));
    Vector3 farWorld(XMVectorGetX(farWorldV), XMVectorGetY(farWorldV), XMVectorGetZ(farWorldV));

    Vector3 rayDir = farWorld - nearWorld;
    if (rayDir.LengthSquared() > 1e-6f) rayDir.Normalize();

    // カメラ前方の平面(法線=camForward)との交差を求める
    Vector3 camForward = GetForward();
    Vector3 planePoint = cameraPos + camForward * m_AimPlaneDistance;
    float denom = rayDir.Dot(camForward);

    Vector3 worldTarget;
    const float EPS = 1e-5f;
    if (fabs(denom) < EPS)
    {
        // 平行に近い場合は近点から ray を伸ばす
        worldTarget = nearWorld + rayDir * m_AimPlaneDistance;
    }
    else
    {
        float t = (planePoint - nearWorld).Dot(camForward) / denom;
        worldTarget = nearWorld + rayDir * t;
    }

    // AimPoint を滑らかに更新（直接代入だと振動しやすい）
    const float aimLerp = 12.0f; // 追従の速さ（調整可）
    m_AimPoint = m_AimPoint + (worldTarget - m_AimPoint) * std::min(1.0f, aimLerp * dt);

    // （任意デバッグ）ここで gDebug などで nearWorld / farWorld / worldTarget / m_AimPoint を描画すると良い

    // ------- lookTarget（カメラの注視点）を m_AimPoint に寄せて計算 -------
    Vector3 aimDir = m_AimPoint - targetPos;
    if (aimDir.LengthSquared() > 1e-6f) aimDir.Normalize();
    else
    {
        // フォールバック：現在のカメラの前方を使う（安全策）
        aimDir = GetForward();
    }

    // 前方へどれだけ注視点を置くか（この値でプレイヤーの画面内位置が変わる）
    Vector3 rawLookTarget = targetPos + aimDir * m_LookAheadDistance + Vector3(0.0f, (m_DefaultHeight + m_AimHeight) * 0.5f, 0.0f);

    // 平滑化（首の挙動）
    m_LookTarget = m_LookTarget + (rawLookTarget - m_LookTarget) * std::min(1.0f, m_LookAheadLerp * dt);

    // ビュー行列はカメラの位置 -> 平滑化された注視点
    m_ViewMatrix = Matrix::CreateLookAt(cameraPos, m_LookTarget, Vector3::Up);

    // レンダラへセット
    Renderer::SetViewMatrix(m_ViewMatrix);
    Renderer::SetProjectionMatrix(m_ProjectionMatrix);
}



void FollowCameraComponent::UpdateCameraPosition()
{
    float dt = Application::GetDeltaTime();

    // 距離と高さはエイムでのみ変化（右クリック）
    float dist = m_IsAiming ? m_AimDistance : m_DefaultDistance;
    float height = m_IsAiming ? m_AimHeight : m_DefaultHeight;

    // --- カメラ位置は「プレイヤーの背後」に置く（プレイヤーの yaw に合わせる）
    // プレイヤーの向きに合わせて behind に配置しておく（首だけ回す設計）
    Vector3 targetPos = m_Target->GetPosition();
    Vector3 targetRot = m_Target->GetRotation(); // rot.y = yaw を想定
    float playerYaw = targetRot.y;

    Matrix playerRot = Matrix::CreateRotationY(playerYaw);
    Vector3 baseOffset = Vector3(0.0f, 0.0f, -dist);
    Vector3 rotatedOffset = Vector3::Transform(baseOffset, playerRot);

    Vector3 desiredPos = targetPos + rotatedOffset + Vector3(0.0f, height, 0.0f);

    // スプリングで位置を滑らかに追従（距離はここで固定される）
    m_Spring.Update(desiredPos, dt);
}

Vector3 FollowCameraComponent::GetForward() const
{
    return m_ViewMatrix.Invert().Forward();
}

Vector3 FollowCameraComponent::GetRight() const
{
    return m_ViewMatrix.Invert().Right();
}
