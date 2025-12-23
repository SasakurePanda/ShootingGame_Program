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
	void SetSpeed(float s) { m_speed = s; }                       //移動速度のセット
	void SetArrivalThreshold(float t) { m_arrivalThreshold = t; } //到着とみなす閾値(到着と判定するウェイポイントを中心にする半径)
	void SetPingPong(bool p) { m_pingPong = p; }                  //往復移動なら true、ループなら false
	void SetLoop(bool loop) { m_loop = loop; }                    //ループするか（pingpongとの組合せに注意）
	void SetStartIndex(size_t idx)			//スタートする地点の番号
	{
		if (!m_waypoints.empty())
		{
			m_currentIndex = idx % m_waypoints.size();
		}
	}	
	void SetFaceMovement(bool v) { m_faceMovement = v; }    //進行方向を向くか
	void SetTurnRate(float turnRate) { m_turnRate = turnRate; }      // 値が大きいほど曲がりやすい
	void SetSlowRadius(float slowRadius) { m_slowRadius = slowRadius; } // ここに入ったら減速開始

	//-------------Get関数--------------
	size_t GetCurrentIndex() const { return m_currentIndex; }				  //今向かっている地点のポイントのインデックスを取得
	const std::vector<Vector3>& GetWaypoints() const { return m_waypoints; }  //設定しているウェイポイントの配列取得

	//リセット
	void Reset();

private:
	//-------------ウェイポイント関連--------------
	std::vector<Vector3> m_waypoints;			//向かう地点(ウェイポイント)を保存するベクターの配列
	size_t m_currentIndex = 0;					//今向かっている地点のインデックス
	bool m_pingPong = true;						//往復かループかのbool(往復移動ならtrue、ループならfalse)
	int m_dir = 1;								//1ならインデックス順に-1ならハンインデックス順に進む
	bool m_loop = true;							//ループをしているかどうかのbool

	//-------------移動・回転関連--------------
	bool m_faceMovement = true;					//進行方向を向いているかどうかのbool					  //
	float m_speed = 6.0f;						//移動速度
	float m_arrivalThreshold = 0.5f;			//到着したと判定するための半径

	Vector3 m_currentDir = Vector3(0.0f,0.0f,1.0f);	// 現在の移動方向ベクトル
	float m_turnRate = 6.0f;					// 曲がる速さ（値が大きいほど曲がりやすい）
	float m_slowRadius = 2.0f;				// この範囲に入ったら減速開始

	//-------------コールバック--------------
	std::function<void(size_t)> m_onReached;

	//-------------内部関数--------------
	void AdvanceWaypoint();
};
