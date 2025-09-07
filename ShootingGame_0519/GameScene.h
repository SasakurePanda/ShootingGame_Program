#pragma once
#include <vector>
#include <d3d11.h>
#include "IScene.h"
#include "FreeCamera.h"
#include "Player.h"
#include "CameraObject.h"
#include "Enemy.h"
#include "TextureComponent.h"
#include "Reticle.h"

//---------------------------------
//ISceneを継承したGameScene
//---------------------------------
class GameScene : public IScene
{
public:
	explicit GameScene() {};
	void Update(float deltatime) override;
	void Draw(float deltatime) override;
	void Init() override;
	void Uninit() override;
	void AddObject(std::shared_ptr<GameObject> obj) override;

private:
	std::shared_ptr<Player> m_player;
	std::shared_ptr<Enemy> m_enemy;
	std::shared_ptr<CameraObject> m_FollowCamera;
	std::vector<std::shared_ptr<GameObject>> m_GameObjects;
	//std::shared_ptr<GridFloor> m_GridFloor;
	 // --- レティクル関係 ---
	std::shared_ptr<GameObject> m_reticleObj;           // レティクル用 GameObject（描画のみでコンポーネント持つ）
	std::shared_ptr<TextureComponent> m_reticleTex;     // レティクルのテクスチャコンポーネント

	bool m_isDragging = false;      // ドラッグ中フラグ
	POINT m_lastDragPos{ 0,0 };       // 最終置いたスクリーン座標

	float m_reticleW = 64.0f;       // レティクル幅（px）
	float m_reticleH = 64.0f;       // レティクル高さ（px）

	void SetReticleByCenter(const POINT& screenPos)
	{
		if (!m_reticleTex) return;
		float x = static_cast<float>(screenPos.x) - m_reticleW * 0.5f;
		float y = static_cast<float>(screenPos.y) - m_reticleH * 0.5f;
		m_reticleTex->SetScreenPosition(x, y);
	}

	std::shared_ptr<Reticle> m_reticle;
};
