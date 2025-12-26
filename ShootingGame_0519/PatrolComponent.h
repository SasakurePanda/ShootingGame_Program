#pragma once
#include "Component.h"
#include <vector>
#include <SimpleMath.h>
#include <functional>

using namespace DirectX::SimpleMath;

/// <summary>
/// 指定している位置まで移動する敵のコンポーネント
/// 今は補完などはないですが、補完スプラインで動かそうと
/// 考えています。
/// </summary>
class PatrolComponent : public Component
{
public:
	PatrolComponent() = default;
	~PatrolComponent() override = default;

	void Initialize() override;
	void Update(float dt) override;

	//-------------Set関数--------------
	void SetWaypoints(const std::vector<Vector3>& pts);
	void SetWaypoints(std::vector<Vector3>&& pts);
	void SetArrivalThreshold(float t) { m_arrivalThreshold = t; }
	void SetSpeed(float s) { m_speed = s; }                       //移動速度のセット
	void SetFaceMovement(bool face) { m_faceMovement = face; }    //進行方向を向くかどうか
	void SetUseSpline(bool useSpline) { m_useSpline = useSpline; }
	void SetSplineTension(float tension) { m_splineTension = tension; }	//スプラインの滑らかさ
	void SetLoop(bool loop) { m_loop = loop; }                    //ループするか（pingpongとの組合せに注意）
	void SetPingPong(bool p) { m_pingPong = p; }
	//-------------Get関数--------------
	size_t GetCurrentIndex() const { return m_currentIndex; }				  //今向かっている地点のポイントのインデックスを取得
	const std::vector<Vector3>& GetWaypoints() const { return m_waypoints; }  //設定しているウェイポイントの配列取得

	//リセット
	void Reset();

private:
	//-------------ウェイポイント関連--------------
	std::vector<Vector3> m_waypoints;			//向かう地点(ウェイポイント)を保存するベクターの配列
	size_t m_currentIndex = 0;					//今向かっている地点のインデックス
	bool m_loop = true;							//ループをしているかどうかのbool

	//-------------移動・回転関連--------------
	bool m_faceMovement = true;					//進行方向を向いているかどうかのbool					  //
	float m_speed = 6.0f;						//移動速度
	bool m_pingPong = true;

	//-------------スプライン関連--------------
	bool m_useSpline = false;					//スプライン補完を使うかどうかのbool
	float m_splineTension = 0.5f;				//スプラインの滑らかさ（0～1）

	float m_segmentT = 0.0f;				//現在のセグメントの長さ
	float m_arrivalThreshold = 0.5f;

	Vector3 m_currentDir = Vector3(0.0f,0.0f,1.0f);	// 現在の移動方向ベクトル
	float m_turnRate = 6.0f;					// 曲がる速さ（値が大きいほど曲がりやすい）
	float m_slowRadius = 2.0f;				// この範囲に入ったら減速開始

	//-------------コールバック--------------
	std::function<void(size_t)> m_onReached;

	//-------------内部関数--------------
	Vector3 GetPointClamped(int index) const;
	Vector3 EvalCatmullRomXZ(const Vector3& p0, const Vector3& p1, const Vector3& p2, const Vector3& p3, float t) const;
	Vector3 EvalCatmullRomTangentXZ(const Vector3& p0, const Vector3& p1, const Vector3& p2, const Vector3& p3, float t) const;

	void AdvanceSegment();
};
