#pragma once
#include <SimpleMath.h>
using namespace DirectX::SimpleMath;

class SpringVector3
{
public:
    SpringVector3() = default;

    //ばねのリセット
    void Reset(const Vector3& pos)
    {
        m_Position = pos;
        m_Velocity = Vector3::Zero;
    }

    void Update(const Vector3& target, float deltaTime)
    {
        if (deltaTime <= 0.0f) return;

        Vector3 displacement = m_Position - target;
        Vector3 springForce = -m_Stiffness * displacement;
        Vector3 dampingForce = -m_Damping * m_Velocity;

        Vector3 force = springForce + dampingForce;
        Vector3 acceleration = force / m_Mass;

        m_Velocity += acceleration * deltaTime;
        m_Position += m_Velocity * deltaTime;
    }

    //剛性(ばねの固さ)のセット関数
    void SetStiffness(float k) 
    {
        m_Stiffness = k;
    }

    //減衰(ばねの)のセット関数
    void SetDamping(float d)
    {
        m_Damping = d; 
    }

    //質量(追尾の反応の速さなどのため)のセット関数
    void SetMass(float m)
    {
        m_Mass = m;
    }

    //今現在のばねの結果とした位置
    Vector3 GetPosition() const { return m_Position; }

private:
    //今の位置を保存する変数
    Vector3 m_Position = Vector3::Zero;

    //今のベクトルを保存する変数
    Vector3 m_Velocity = Vector3::Zero;

    //剛性
    float m_Stiffness = 10.0f;
    //減衰
    float m_Damping = 5.0f;
    //質量
    float m_Mass = 1.0f;
};

