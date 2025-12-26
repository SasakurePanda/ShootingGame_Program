#include "PatrolComponent.h"
#include "GameObject.h"
#include <algorithm>
#include <cmath>

using namespace DirectX::SimpleMath;

void PatrolComponent::Initialize()
{
	//向かう地点が最低一つは設定されており、かつ今設定されているインデックスが
	//全ウェイポイント数以上の場合
	if (!m_waypoints.empty() && m_currentIndex >= m_waypoints.size())
	{
		m_currentIndex = 0;
	}
}

void PatrolComponent::Update(float dt)
{
	//親オブジェクトがないなら
	if (!GetOwner()){return;}
	//向かう地点が何もない場合
	if (m_waypoints.size() < 2) { return; }
	//デルタタイムが0より小さい場合
	if (dt <= 0.0f) { return; }

	GameObject* owner = GetOwner();

	if (!m_useSpline)
	{
		//ここは既存の直進版を呼ぶ
		return;
	}

	int i1 = static_cast<int>(m_currentIndex);
	int i2 = i1 + 1;
	int i0 = i1 - 1;
	int i3 = i2 + 1;

	Vector3 p0 = GetPointClamped(i0);
	Vector3 p1 = GetPointClamped(i1);
	Vector3 p2 = GetPointClamped(i2);
	Vector3 p3 = GetPointClamped(i3);

	Vector3 tangent = EvalCatmullRomTangentXZ(p0, p1, p2, p3, m_segmentT);
	float tangentLen = tangent.Length();

	if (tangentLen < 1e-6f)
	{
		AdvanceSegment();
		return;
	}

	//等速で進むようにtを計算する
	float deltaT = (m_speed * dt) / tangentLen;
	m_segmentT += deltaT;

	//区間をまたぐ処理
	while (m_segmentT >= 1.0f)
	{
		m_segmentT -= 1.0f;
		AdvanceSegment();

		int count = static_cast<int>(m_waypoints.size());

		int i1 = static_cast<int>(m_currentIndex);
		int i2 = i1 + 1;

		if (m_loop)
		{
			i2 = (i2 % count);
		}

		int i0 = i1 - 1;
		int i3 = i2 + 1;

		Vector3 p0 = GetPointClamped(i0);
		Vector3 p1 = GetPointClamped(i1);
		Vector3 p2 = GetPointClamped(i2);
		Vector3 p3 = GetPointClamped(i3);
	}

	Vector3 newPos = EvalCatmullRomXZ(p0, p1, p2, p3, m_segmentT);

	Vector3 pos = owner->GetPosition();
	newPos.y = pos.y;

	owner->SetPosition(newPos);

	if (m_faceMovement)
	{
		Vector3 dir = EvalCatmullRomTangentXZ(p0, p1, p2, p3, m_segmentT);
		dir.y = 0.0f;

		if (dir.LengthSquared() > 1e-6f)
		{
			dir.Normalize();
			float yaw = std::atan2(dir.x, dir.z);

			Vector3 rot = owner->GetRotation();
			rot.y = yaw;
			owner->SetRotation(rot);
		}
	}

	static float s_logTimer = 0.0f;
	s_logTimer += dt;
}

/// <summary>
/// 向かう地点を配列から取り出す関数
/// </summary>
/// <param name="index"> 欲しい地点の配列番号 </param>
/// <returns>指定した配列番号の地点座標</returns>
Vector3 PatrolComponent::GetPointClamped(int index) const
{
	//サイズを取得
	int count = static_cast<int>(m_waypoints.size());

	//ループなら
	if (m_loop)
	{
		//
		int wrapped = index % count;
		if (wrapped < 0)
		{
			wrapped += count;
		}

		return m_waypoints[wrapped];
	}

	//ループじゃないときは
	int clamped = std::clamp(index, 0, count - 1);
	return m_waypoints[clamped];
}

/// <summary>
/// 4つの地点ととから、曲線上の位置（座標）を計算する関数
/// </summary>
/// <param name="p0">p1 の1つ前の地点</param>
/// <param name="p1">今の区間のスタート地点</param>
/// <param name="p2">今の区間のゴール地点</param>
/// <param name="p3">p2 の1つ後の地点</param>
/// <param name="t">移動の進み具合</param>
/// <returns>敵の進む座標</returns>
Vector3 PatrolComponent::EvalCatmullRomXZ(const Vector3& p0, const Vector3& p1, const Vector3& p2, const Vector3& p3, float t) const
{
	float t2 = t * t;
	float t3 = t2 * t;

	Vector3 pos =
		0.5f * (
			(2.0f * p1) +
			(-p0 + p2) * t +
			(2.0f * p0 - 5.0f * p1 + 4.0f * p2 - p3) * t2 +
			(-p0 + 3.0f * p1 - 3.0f * p2 + p3) * t3
			);

	pos.y = 0.0f;
	return pos;
}

Vector3 PatrolComponent::EvalCatmullRomTangentXZ(const Vector3& p0, const Vector3& p1, const Vector3& p2, const Vector3& p3, float t) const
{
	float t2 = t * t;

	Vector3 tan =0.5f * ((-p0 + p2) +
					      2.0f * (2.0f * p0 -
					       5.0f * p1 + 4.0f * p2 -
						  p3) * t +3.0f * 
						  (-p0 + 3.0f * p1 - 3.0f * 
						   p2 + p3) * t2);

	tan.y = 0.0f;
	return tan;
}

void PatrolComponent::AdvanceSegment()
{
	int count = static_cast<int>(m_waypoints.size());
	if (count < 2)
	{
		return;
	}

	if (m_loop)
	{
		m_currentIndex = (m_currentIndex + 1) % static_cast<size_t>(count);
		return;
	}

	// 非ループ：最後の区間開始は count-2 まで
	if (m_currentIndex + 1 < static_cast<size_t>(count - 1))
	{
		m_currentIndex++;
		return;
	}

	m_currentIndex = static_cast<size_t>(count - 2);
	m_segmentT = 1.0f;
}

void PatrolComponent::SetWaypoints(const std::vector<Vector3>& pts)
{
	m_waypoints = pts;
	Reset();
}

void PatrolComponent::SetWaypoints(std::vector<Vector3>&& pts)
{
	m_waypoints = std::move(pts);
	Reset();
}

void PatrolComponent::Reset()
{
	m_currentIndex = 0;
	m_segmentT = 0.0f;
	m_splineTension = 0.5f;
}
