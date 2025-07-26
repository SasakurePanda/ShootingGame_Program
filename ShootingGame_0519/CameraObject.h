#pragma once
#include "GameObject.h"
#include <DirectXMath.h>
#include <SimpleMath.h>
#include "FollowCameraComponent.h"

using namespace DirectX;

class CameraObject : public GameObject
{
public:
	SimpleMath::Matrix viewMatrix;
	SimpleMath::Matrix projMatrix;

	CameraObject() = default;
	~CameraObject() override = default;

	void Initialize() override;

	std::shared_ptr<FollowCameraComponent> GetCameraComponent() const
	{
		return m_FollowCameraComponent;
	}

	//void Update() override;
private:
	std::shared_ptr<FollowCameraComponent> m_FollowCameraComponent;

	//2025/07/23 Sasakure
	//時間があればComponentクラスを継承したFollowCameraComponentではなく
	//Componentクラスを継承したBaseCameraComponentを継承したFollowCameraComponentにするとかを検討
	//そしたら中身の差し替え簡単かなって……
	
};
