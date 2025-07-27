#include "FollowCameraComponent.h"
#include "Renderer.h"
#include "Application.h"
#include "Input.h"

FollowCameraComponent::FollowCameraComponent()
{
    // Spring 設定（初期化時に一度だけ）
    m_Spring.SetStiffness(12.0f); // バネの強さ
    m_Spring.SetDamping(6.0f);    // ダンパーの強さ
    m_Spring.SetMass(1.0f);       // 質量（基本1.0fでOK）

    // 初期プロジェクション行列（固定）
    //画面幅を取得し、floatの変数に保存
    float width = static_cast<float>(Application::GetWidth());

    //画面高さを取得し。floatの変数に保存
    float height = static_cast<float>(Application::GetHeight());

    //透視投影行列を作る関数(3Dを2Dに落とし込むよ)
    m_ProjectionMatrix = Matrix4x4::CreatePerspectiveFieldOfView(XMConvertToRadians(45.0f), width / height, 0.1f, 1000.0f);
}

void FollowCameraComponent::Update()//
{
    if (!m_Target) return;

    //-------------------------
    //マウスから角度を取得
    //-------------------------
    POINT delta = Input::GetMouseDelta();

    float sensitivity = 0.01f; // 必要に応じて調整
    // 回転角度更新
    m_Yaw += delta.x * sensitivity;
    m_Pitch += delta.y * sensitivity;

    // 回転制限（ピッチ：上下）
    float pitchLimit = XMConvertToRadians(0.0f); // 上下最大 ±80度
    m_Pitch = std::clamp(m_Pitch, -pitchLimit, pitchLimit);

    // 任意：Yaw制限（左右最大 ±120度など）
    float yawLimit = XMConvertToRadians(40.0f); // 例：制限したいなら
    m_Yaw = std::clamp(m_Yaw, -yawLimit, yawLimit);

    //-------------------------------
    //回転に基づいてカメラ位置を計算
    //-------------------------------
    Vector3 targetPos = m_Target->GetPosition();

    Matrix rotY = Matrix::CreateRotationY(m_Yaw);
    Matrix rotX = Matrix::CreateRotationX(m_Pitch);
    Matrix rotation = rotX * rotY;

    // カメラの相対位置（Z方向に後ろ）
    Vector3 offset = Vector3(0, 0, -m_Distance);
    Vector3 rotatedOffset = Vector3::Transform(offset, rotation);

    Vector3 desiredPos = targetPos + rotatedOffset + Vector3(0, m_Height, 0);

    // バネ更新
    float deltaTime = 1.0f / 60.0f; // 固定でもOK
    m_Spring.Update(desiredPos, deltaTime);

    Vector3 cameraPos = m_Spring.GetPosition();

    // ビュー行列を生成
    m_ViewMatrix = Matrix4x4::CreateLookAt(cameraPos, targetPos, Vector3::Up);

    // レンダラに渡す
    Renderer::SetViewMatrix(m_ViewMatrix);
    Renderer::SetProjectionMatrix(m_ProjectionMatrix);
}


