#pragma once
#include <DirectXMath.h>
#include <SimpleMath.h>
using namespace DirectX;
//--------------------------------------------------------------------------------
//BaseCamera関数
//主なカメラの元になるクラスを作りたい
//--------------------------------------------------------------------------------
class BaseCamera
{
protected:
	//カメラの座標位置
	SimpleMath::Vector3	m_position = SimpleMath::Vector3(0.0f, 0.0f, 0.0f);
	//注視点(カメラの見る点(座標)として向く先の座標)
	SimpleMath::Vector3 m_lookat{};
	//ビュー行列
	//(カメラの位置・向きからワールド座標をカメラ視点に変換する行列)
	SimpleMath::Matrix m_viewmtx{};
	//プロジェクション行列
	//射影行列(3D → 2D投影を行う行列)
	SimpleMath::Matrix m_projmtx{};

public:
	BaseCamera() = default;
	virtual ~BaseCamera() noexcept = default;

	virtual void Init() = 0;

	virtual void Update(uint64_t delta) = 0;

	virtual void Draw(uint64_t delta) = 0;

	//カメラの座標のゲッター関数
	virtual XMMATRIX GetViewMatrix() const = 0;

	//カメラの注視点のゲッター関数
	virtual XMMATRIX GetProjectionMatrix() const = 0;

	//カメラの座標のセッター関数
	virtual void SetPosition(const SimpleMath::Vector3& position) = 0;

	//カメラの注視点のセッター関数
	virtual void SetLookat(const SimpleMath::Vector3& Lookat) = 0;

};

