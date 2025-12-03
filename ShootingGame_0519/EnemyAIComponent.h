#pragma once
#include "Component.h"
#include <SimpleMath.h>

class PlayAreaComponent;

class EnemyAIComponent : public Component
{
public:
    EnemyAIComponent();

    void Initialize() override;
    void Update(float dt) override;

    //セッター関数
    void SetMaxSpeed(float s) { m_maxSpeed = s; }
    void SetMaxForce(float f) { m_maxForce = f; }
    void SetFleeStrength(float s) { m_fleeStrength = s; }
    void SetLookahead(float l) { m_lookahead = l; }
    void SetFeelerCount(int n) { m_feelerCount = n; }
	void SetFeelerSpread(float a) { m_feelerSpread = a; }
	void SetTarget(GameObject* player) { m_target = player; }

	void SetPlayArea(PlayAreaComponent* playArea) { m_playArea = playArea; }

	void SetPreferredHeightBand(float minY, float maxY)
	{
		m_preferredMinY = minY;
		m_preferredMaxY = maxY;
		m_baseHeight = (minY + maxY) * 0.5f;
	}

private:
	//逃げる力を計算する関数
    DirectX::SimpleMath::Vector3 ComputeFlee(const DirectX::SimpleMath::Vector3& pos);

	//ターゲットから逃げる速度を計算する関数
	DirectX::SimpleMath::Vector3 ComputeFleeVelocity(const DirectX::SimpleMath::Vector3& pos) const;
	
    //障害物回避の力を計算する関数
    DirectX::SimpleMath::Vector3 ComputeAvoidance(const DirectX::SimpleMath::Vector3& pos, const DirectX::SimpleMath::Vector3& forward);

	//境界回避の力を計算する関数
	DirectX::SimpleMath::Vector3 ComputeBoundaryAvoid(const DirectX::SimpleMath::Vector3& pos) const;

	//高さ調整の力を計算する関数
	DirectX::SimpleMath::Vector3 ComputeHeightControl(const DirectX::SimpleMath::Vector3& pos) const;

	float m_maxSpeed = 20.0f;       //最大速度
	float m_maxForce = 10.0f;       //最大力
	float m_fleeStrength = 1.0f;    //逃げる力の強さ
	float m_lookahead = 40.0f;      //先読み距離
	int   m_feelerCount = 5;        //フィールアーの数
	float m_feelerSpread = DirectX::XM_PI / 4.0f;   //フィールアーの広がり角度

	float m_boundaryMargin = 30.0f;	//境界に近づいたとみなす距離
	float m_boundaryWeight = 9.0f;	//どれぐらい強く境界から回避し内側に戻るか

	float m_preferredMinY = 80.0f;		//滞在してほしい地点の下限
	float m_preferredMaxY = 130.0f;		//滞在してほしい地点の上限
	float m_baseHeight    = 100.0f;		//基準となる高さ

	float m_verticalFleeScale = 0.2f;	 //垂直方向の逃げる力の強さ倍率
	float m_heightSteerStrength = 0.3f;  //高さ調整の強さ

	DirectX::SimpleMath::Vector3 m_lastPos = { 0,0,0 };
	float m_stuckTimer = 0.0f;
	bool  m_inEscapeMode = false;
	float m_escapeTime = 0.0f;
	DirectX::SimpleMath::Vector3 m_escapeDir = { 0,0,0 };

	PlayAreaComponent* m_playArea = nullptr;	//プレイエリアのクラスのポインタ

	DirectX::SimpleMath::Vector3 m_velocity = { 0,0,0 };    //現在の速度

	GameObject* m_target = nullptr;  //プレイヤーオブジェクトへのポインタ
};