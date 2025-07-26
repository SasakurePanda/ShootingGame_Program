#pragma once
#include "BaseCamera.h"
#include "Input.h"

class FreeCamera : public BaseCamera
{
public:
    FreeCamera(); //コンストラクタ

    void Init() override;         //初期化関数
    void Update(uint64_t delta) override;   //更新関数
    void Draw(uint64_t delta) override;     //描画関連関数

    XMMATRIX GetViewMatrix() const override { return m_viewmtx; }
    XMMATRIX GetProjectionMatrix() const override { return m_projmtx; }

    const DirectX::SimpleMath::Vector3& GetPosition() const { return m_position; }

    void SetPosition(const DirectX::SimpleMath::Vector3& pos) override { m_position = pos; }
    void SetLookat(const DirectX::SimpleMath::Vector3& lookat) override { m_target = lookat; }

private:
    void UpdateViewMatrix();
    void UpdateProjectionMatrix();
    void HandleInput(float deltaTime);

    // 状態
    DirectX::SimpleMath::Vector3 m_position;
    DirectX::SimpleMath::Vector3 m_target;
    float m_alpha;
    float m_beta;
    float m_radius;

    // 入力による移動速度
    float m_moveSpeed = 5.0f;
    float m_rotateSpeed = 0.5f;
    float m_zoomSpeed = 2.0f;
};
