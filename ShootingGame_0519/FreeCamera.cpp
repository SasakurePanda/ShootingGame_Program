#include <iostream>
#include <algorithm>
#include "FreeCamera.h"
#include "Application.h"
#include "renderer.h"

using namespace DirectX;
using namespace DirectX::SimpleMath;

FreeCamera::FreeCamera():m_position(0, 0, -3),m_target(0, 0, 0),m_alpha(0.0f),m_beta(XM_PIDIV2),m_radius(5.0f)
{


}

void FreeCamera::Init() 
{

}

void FreeCamera::Update(uint64_t delta)
{
    //Input::Update();  // 先に状態を更新
    float deltaTime = static_cast<float>(delta) / 1000.0f;

    // マウス位置とデルタを std::cout で出力
    POINT mp = Input::GetMousePosition();
    POINT md = Input::GetMouseDelta();
    std::cout << "dt=" << deltaTime
        << "  MousePos=(" << mp.x << "," << mp.y << ")"
        << "  Delta=(" << md.x << "," << md.y << ")"
        << std::endl;

    HandleInput(deltaTime);
    UpdateViewMatrix();
    UpdateProjectionMatrix();
}

void FreeCamera::Draw(uint64_t) 
{

}

void FreeCamera::UpdateViewMatrix()
{
    float x = m_radius * sinf(m_beta) * cosf(m_alpha);
    float y = m_radius * cosf(m_beta);
    float z = m_radius * sinf(m_beta) * sinf(m_alpha);
    m_position = m_target + Vector3(x, y, z);

    m_viewmtx = Matrix::CreateLookAt(m_position, m_target, Vector3::Up);

    // 2. 行列を取得してRendererに渡す
    Renderer::SetViewMatrix(m_viewmtx);
}

void FreeCamera::UpdateProjectionMatrix()
{
    constexpr float fov = XMConvertToRadians(60.0f);
    float aspect = static_cast<float>(Application::GetWidth()) / Application::GetHeight();
    m_projmtx = Matrix::CreatePerspectiveFieldOfView(PI / 4, aspect, 0.1f, 100.0f);

    Renderer::SetProjectionMatrix(m_projmtx);
}

void FreeCamera::HandleInput(float deltaTime)
{
    // 方向キーで視点回転
    if (Input::IsKeyDown('J'))
    {
        std::cout << "Jが押されました\n";
        m_alpha -= m_rotateSpeed * deltaTime;
    }
    if (Input::IsKeyDown('L'))
    {
        m_alpha += m_rotateSpeed * deltaTime;
    }
    if (Input::IsKeyDown('I'))
    {
        m_beta -= m_rotateSpeed * deltaTime;
        if (m_beta < 0.01f) m_beta = 0.01f; // 直上は避ける
    }
    if (Input::IsKeyDown('K'))
    {
        m_beta += m_rotateSpeed * deltaTime;
        if (m_beta > XM_PI - 0.01f) m_beta = XM_PI - 0.01f; // 直下も避ける
    }

    // 視点回転（右ドラッグ or 矢印キー）
    if (Input::IsMouseLeftDown())
    {
        //std::cout << "カメラ側でQキー押せています\n";
        POINT delta = Input::GetMouseDelta();
        static float prevDx = 0, prevDy = 0;
        float alpha = 0.2f;  // 0.0~1.0 の重み
        float smoothDx = prevDx * (1 - alpha) + delta.x * alpha;
        float smoothDy = prevDy * (1 - alpha) + delta.y * alpha;
        prevDx = smoothDx; prevDy = smoothDy;
        m_alpha += smoothDx * m_rotateSpeed * deltaTime;
        m_beta += smoothDy * m_rotateSpeed * deltaTime;


    }

    // ズームイン/アウト
    if (Input::IsKeyDown('Q'))
    {
        std::cout << "カメラ側でQキー押せています\n";
        m_radius -= m_zoomSpeed * deltaTime;
    }
    if (Input::IsKeyDown('E'))
    {
        std::cout << "カメラ側でEキー押せています\n";
        m_radius += m_zoomSpeed * deltaTime;
    }
    m_radius = std::clamp(m_radius, 1.0f, 100.0f);

    // 平行移動（WASD）
    Vector3 forward = (m_target - m_position);
    forward.y = 0;
    forward.Normalize();
    Vector3 right = forward.Cross(Vector3::Up);

    Vector3 move{};
    if (Input::IsKeyDown('I')) move += forward;
    if (Input::IsKeyDown('K')) move -= forward;
    if (Input::IsKeyDown('J')) move -= right;
    if (Input::IsKeyDown('L')) move += right;
    if (Input::IsKeyDown(VK_SPACE)) move += Vector3::Up;
    if (Input::IsKeyDown(VK_RETURN)) move -= Vector3::Up;

    if (move.LengthSquared() > 0.0f)
    {
        move.Normalize();
        Vector3 delta = move * m_moveSpeed * deltaTime;
        m_target += delta;
    }
}
