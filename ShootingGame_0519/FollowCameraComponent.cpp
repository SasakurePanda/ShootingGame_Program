#include "FollowCameraComponent.h"
#include "Renderer.h"
#include "Application.h"

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

    Vector3 targetPos = m_Target->GetPosition();
    Vector3 desiredPos = targetPos + Vector3(0, m_Height, -m_Distance);

    float deltaTime = 1.0f / 60.0f; // 固定でもよいが、Application::GetDeltaTime()でもOK
    m_Spring.Update(desiredPos, deltaTime);

    Vector3 cameraPos = m_Spring.GetPosition();
    m_ViewMatrix = Matrix4x4::CreateLookAt(cameraPos, targetPos, Vector3::Up);

    Renderer::SetViewMatrix(m_ViewMatrix);
    Renderer::SetProjectionMatrix(m_ProjectionMatrix);
}

void FollowCameraComponent::SetTarget(GameObject* target)
{
    m_Target = target;
    if (target)
    {
        Vector3 initial = target->GetPosition() + Vector3(0, m_Height, -m_Distance);
        m_Spring.Reset(initial);
    }
}

void FollowCameraComponent::SetDistance(float dist)
{
    m_Distance = dist;
}

void FollowCameraComponent::SetHeight(float h)
{
    m_Height = h;
}

//ダンピングばね