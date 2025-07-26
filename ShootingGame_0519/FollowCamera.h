#pragma once
#include "BaseCamera.h"
#include "Input.h"
#include <SimpleMath.h>
#include <memory>

class GameObject; // 前方宣言（実体を後で定義）

class FollowCamera : public BaseCamera
{
public:
    FollowCamera();
    ~FollowCamera() noexcept override = default;

    void Init() override;
    void Update(uint64_t delta) override;
    void Draw(uint64_t delta) override {}

    XMMATRIX GetViewMatrix() const override { return m_viewmtx; }
    XMMATRIX GetProjectionMatrix() const override { return m_projmtx; }

    void SetPosition(const DirectX::SimpleMath::Vector3& pos) override { m_position = pos; }
    void SetLookat(const DirectX::SimpleMath::Vector3& lookat) override { m_lookat = lookat; }

    void SetTarget(std::shared_ptr<GameObject> player) { m_player = player; }

private:
    std::weak_ptr<GameObject> m_player;

    float m_distance = 5.0f;     // 後方距離
    float m_height = 2.0f;       // 上方オフセット
    float m_yaw = 0.0f;
    float m_pitch = 0.3f;
    float m_smoothSpeed = 0.1f;

    DirectX::SimpleMath::Vector3 m_currentPos;

    void HandleMouse(float deltaTime);
    void UpdateViewMatrix();
    void UpdateProjectionMatrix();
};