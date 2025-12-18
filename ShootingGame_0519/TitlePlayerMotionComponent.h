#pragma once
#include "Component.h"
#include <SimpleMath.h>

class TitlePlayerMotionComponent : public Component
{
public:
	TitlePlayerMotionComponent() = default;
	~TitlePlayerMotionComponent() = default;
	void Initialize();
	void Update(float dt);

    //--------Set関数-------
    void SetBasePosition(const DirectX::SimpleMath::Vector3& pos) { m_BasePos = pos; }
    void SetRotateSpeed(float radPerSec) { m_RotateSpeed = radPerSec; }
    void SetBobAmplitude(float amp) { m_BobAmplitude = amp; }
	void SetDuration(float duration) { m_Duration = duration; }
    void SetBobSpeed(float speed) { m_BobSpeed = speed; }
    void SetControlPoints(const DirectX::SimpleMath::Vector3& p0,
                          const DirectX::SimpleMath::Vector3& p1,
                          const DirectX::SimpleMath::Vector3& p2,
                          const DirectX::SimpleMath::Vector3& p3);
    //--------Get関数-------
    float GetTime() const { return m_Time; }

private:
    //-------演出関連----------
    float m_Time = 0.0f;

	float m_Duration = 1.5f; //移動にかかる時間

    DirectX::SimpleMath::Vector3 m_BasePos = DirectX::SimpleMath::Vector3(0.0f, 0.0f, 10.0f);

	//-----三次ベジェ曲線の制御点-----
	DirectX::SimpleMath::Vector3 m_p01 = DirectX::SimpleMath::Vector3(0.0f, 0.0f, 0.0f);
	DirectX::SimpleMath::Vector3 m_p02 = DirectX::SimpleMath::Vector3(0.0f, 0.0f, 0.0f);
	DirectX::SimpleMath::Vector3 m_p03 = DirectX::SimpleMath::Vector3(0.0f, 0.0f, 0.0f);
	DirectX::SimpleMath::Vector3 m_p04 = DirectX::SimpleMath::Vector3(0.0f, 0.0f, 0.0f);

    DirectX::SimpleMath::Vector3 EvaluateBezier(float t) const;

    float m_RotateSpeed = 0.6f;
    float m_BobAmplitude = 0.5f;   
    float m_BobSpeed = 2.0f;
};
