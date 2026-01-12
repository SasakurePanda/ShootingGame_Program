#pragma once
#include "Component.h"
#include <SimpleMath.h>
#include <memory>

using namespace DirectX::SimpleMath;

class GameObject;
class BulletComponent;

class HomingComponent : public Component
{
public:
    HomingComponent() = default;
    ~HomingComponent() override = default;

    //------------Set関数--------------
    void SetTarget(const std::weak_ptr<GameObject>& t) { m_target = t; }
    void SetTimeToIntercept(float t) { m_timeToIntercept = t; }
    void SetMaxAcceleration(float a) { m_maxAcceleration = a; }
    void SetLifeTime(float sec) { m_lifeTime = sec; }
    void SetAimBias(const Vector3& b) { m_aimBias = b; }
    void SetAimBiasStrength(float s) { m_aimBiasStrength = s; }
    void SetAimBiasDecay(float d) { m_aimBiasDecay = d; }

    //------------Get関数--------------
    std::weak_ptr<GameObject> GetTarget() const { return m_target; }
    float GetTimeToIntercept() const { return m_timeToIntercept; }
    float GetMaxAcceleration() const { return m_maxAcceleration; }

    //--------その他関数-------
    void Initialize() override;
    void Update(float dt) override;

private:
    //--------------追尾関連------------------
    std::weak_ptr<GameObject> m_target;
    float m_timeToIntercept = 1.5f;   // デフォルト 1 秒で命中を目指す
    float m_maxAcceleration = 400.0f;   // 0 = 無制限、>0 で制限
    float m_lifeTime = 5.0f;          // Homing の寿命（optional）
    float m_age = 0.0f;               // 経過時間

    Vector3 m_prevTargetPos = Vector3::Zero;
    bool m_havePrevTargetPos = false;
    float m_maxTurnRateDeg = 120.0f;

    Vector3 m_aimBias = Vector3::Zero;      // 発射時のランダムバイアス（方向ベクトル）
    float m_aimBiasStrength = 0.0f;         // バイアスの強さ（加算量のスカラー）
    float m_aimBiasDecay = 1.0f;            // 1 秒あたりの減衰量（強さが 0 になるまで減る）
};