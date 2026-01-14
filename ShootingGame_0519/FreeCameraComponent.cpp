#define NOMINMAX
#include "FreeCameraComponent.h"
#include "Application.h"
#include "Input.h"
#include "Renderer.h"
#include <algorithm>
#include <iostream>

using namespace DirectX;
using namespace DirectX::SimpleMath;

Vector3 FreeCameraComponent::GetForward() const
{
    return m_ViewMatrix.Invert().Forward();
}

Vector3 FreeCameraComponent::GetRight() const
{
    return m_ViewMatrix.Invert().Right();
}

Vector3 FreeCameraComponent::GetAimPoint() const
{
    return m_Position + GetForward() * m_AimDistance;
}

Vector3 FreeCameraComponent::GetAimDirectionFromReticle() const
{
    return GetForward();
}

Vector2 FreeCameraComponent::GetReticleScreen() const
{
    float screenW = static_cast<float>(Application::GetWidth());
    float screenH = static_cast<float>(Application::GetHeight());
    return Vector2(screenW * 0.5f, screenH * 0.5f);
}

void FreeCameraComponent::Initialize()
{
    UpdateProjectionIfNeeded();
    m_Position = Vector3(0.0f, 0.0f, 0.0f);
    m_ViewMatrix = Matrix::CreateLookAt(
        m_Position,
        m_Position + Vector3::Forward,
        Vector3::Up
    );
}

void FreeCameraComponent::Update(float dt)
{
    if (dt <= 0.0f)
    {
        return;
    }

    UpdateProjectionIfNeeded();

    Vector3 forward = Vector3::Forward;
    Vector3 right = Vector3::Right;

    float speed = m_MoveSpeed;
    if (Input::IsKeyDown(VK_SHIFT))
    {
        speed = m_BoostSpeed;
    }

    Vector3 move = Vector3::Zero;

    if (Input::IsKeyDown('W'))
    {
        move += forward;
    }
    if (Input::IsKeyDown('S'))
    {
        move -= forward;
    }
    if (Input::IsKeyDown('D'))
    {
        move += right;
    }
    if (Input::IsKeyDown('A'))
    {
        move -= right;
    }
    if (Input::IsKeyDown(VK_SPACE))
    {
        move += Vector3::Up;
    }
    if (Input::IsKeyDown(VK_CONTROL))
    {
        move -= Vector3::Up;
    }

    if (move.LengthSquared() > 1e-6f)
    {
        move.Normalize();
        m_Position += move * speed * dt;
    }

    m_ViewMatrix = Matrix::CreateLookAt(
        m_Position,
        m_Position + Vector3::Forward,
        Vector3::Up
    );


    Renderer::SetViewMatrix(m_ViewMatrix);
    Renderer::SetProjectionMatrix(m_ProjectionMatrix);

    //--------------向き確認ログ（デバッグ用）------------------
    const Vector3 forward01 = GetForward();
    /*std::cout << "CamPos(" << m_Position.x << "," << m_Position.y << "," << m_Position.z << ") "
        << "Forward(" << forward01.x << "," << forward01.y << "," << forward01.z << ")\n";*/

}



void FreeCameraComponent::UpdateViewMatrix()
{
    Matrix rot = Matrix::CreateFromYawPitchRoll(m_Yaw, m_Pitch, 0.0f);
    Vector3 forward = Vector3::TransformNormal(Vector3::Forward, rot);
    if (forward.LengthSquared() > 1e-6f)
    {
        forward.Normalize(); 
    }

    Vector3 lookAt = m_Position + forward;
    m_ViewMatrix = Matrix::CreateLookAt(m_Position, lookAt, Vector3::Up);
}
