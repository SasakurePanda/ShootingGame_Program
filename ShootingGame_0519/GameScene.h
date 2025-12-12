#pragma once
#include <vector>
#include <d3d11.h>
#include "IScene.h"
#include "FreeCamera.h"
#include "Player.h"
#include "CameraObject.h"
#include "Enemy.h"
#include "TextureComponent.h"
#include "DebugRenderer.h"
#include "Reticle.h"
#include "SkyDome.h"
#include "Bullet.h"
#include "TitleBackGround.h"
#include "DebugUI.h"
#include "HPBar.h"
#include "EnemySpawner.h"
#include "BuildingSpawner.h"
#include "PlayAreaComponent.h"
#include "MoveComponent.h"

//---------------------------------
//ISceneを継承したGameScene
//---------------------------------
class GameScene : public IScene
{
public:
	explicit GameScene() {};
	
	//更新関数
	void Update(float deltatime) override;
	//描画関数
	void Draw(float deltatime) override;
	//3Dワールド上の描画関数
	void DrawWorld(float deltatime) override;
	//UI上の描画関数
	void DrawUI(float deltatime) override;
	//初期化関数
	void Init() override;
	//終了関数
	void Uninit() override;

	//モード変更用関数
	void DebugCollisionMode();

	//モード変更用関数
	void DebugSetPlayerSpeed();

	//モード変更用関数
	void DebugSetAimDistance();
	
	//オブジェクトの追加要求関数
	void AddObject(std::shared_ptr<GameObject> obj) override;

	//2Dオブジェクトの追加要求関数
	void AddTextureObject(std::shared_ptr<GameObject> obj);
	
	//オブジェクトの削除要求関数
	void RemoveObject(std::shared_ptr<GameObject>) override {};
	
	//オブジェクトの削除要求関数
	void RemoveObject(GameObject* obj);

	//実際に削除などをする関数
	void FinishFrameCleanup() override;

	bool Raycast(const DirectX::SimpleMath::Vector3& origin,
				 const DirectX::SimpleMath::Vector3& dir,
				 float maxDistance,
				 RaycastHit& outHit,
				 std::function<bool(GameObject*)> predicate = nullptr,
				 GameObject* ignore = nullptr);

	bool RaycastForAI(const DirectX::SimpleMath::Vector3& origin,
					  const DirectX::SimpleMath::Vector3& dir,
					  float maxDistance,
				      RaycastHit& outHit,
					  GameObject* ignore = nullptr);

	const std::vector<std::shared_ptr<GameObject>>& GetObjects() const override { return m_GameObjects; }

	PlayAreaComponent* GetPlayArea() const { return m_playArea.get(); }

	//削除予定のオブジェクトの配列
	std::vector<std::shared_ptr<GameObject>> m_DeleteObjects;

	//追加予定のオブジェクトの配列
	std::vector<std::shared_ptr<GameObject>> m_AddObjects;

private:
	float m_aimMarginX = 80.0f;  // 左右マージン（好きな値に変えてOK）
	float m_aimMarginY = 45.0f;   // 上下マージン

	std::unique_ptr<EnemySpawner> m_enemySpawner;

	std::unique_ptr<BuildingSpawner> m_buildingSpawner;

	std::unique_ptr<DebugRenderer> m_debugRenderer;

	std::shared_ptr<Player> m_player;
	std::shared_ptr<CameraObject> m_FollowCamera;
	std::shared_ptr<SkyDome> m_SkyDome;
	
	//GameScene内の3Dオブジェクトの配列
	std::vector<std::shared_ptr<GameObject>> m_GameObjects;

	//GameScene内の2Dオブジェクトの配列
	std::vector<std::shared_ptr<GameObject>> m_TextureObjects;

	 // --- レティクル関係 ---
	std::shared_ptr<GameObject> m_reticleObj;           // レティクル用 GameObject（描画のみでコンポーネント持つ）
	std::shared_ptr<HPBar> m_HPObj;           // レティクル用 GameObject（描画のみでコンポーネント持つ）
	std::shared_ptr<TextureComponent> m_reticleTex;     // レティクルのテクスチャコンポーネント

	bool m_isDragging = false;      // ドラッグ中フラグ
	POINT m_lastDragPos{ 0,0 };     // 最終置いたスクリーン座標

	float m_reticleW = 64.0f;       // レティクル幅（px）
	float m_reticleH = 64.0f;       // レティクル高さ（px）

	//SceneのUpdate時に追加予定であったオブジェクトの配列の追加などを行う関数
	void SetSceneObject();

	void SetReticleByCenter(const POINT& screenPos)
	{
		if (!m_reticleTex) { return; }

		float screenW = static_cast<float>(Application::GetWidth());
		float screenH = static_cast<float>(Application::GetHeight());

		// --- 内側の枠を定義 ---
		float minX = m_aimMarginX;
		float maxX = screenW - m_aimMarginX;
		float minY = m_aimMarginY;
		float maxY = screenH - m_aimMarginY;

		// マウスが指した中心候補
		float cx = static_cast<float>(screenPos.x);
		float cy = static_cast<float>(screenPos.y);

		// ★ 枠の内側にクランプ（ここが肝）
		cx = std::clamp(cx, minX, maxX);
		cy = std::clamp(cy, minY, maxY);

		// 1) レティクル画像の左上座標に変換してセット
		float x = cx - m_reticleW * 0.5f;
		float y = cy - m_reticleH * 0.5f;
		m_reticleTex->SetScreenPosition(x, y);

		// 2) カメラにも“中心座標”を教える
		if (auto camera = m_player->GetComponent<FollowCameraComponent>())   // Init でセットしておく
		{
			camera->SetReticleScreen(DirectX::SimpleMath::Vector2(cx, cy));
		}
	}

	std::shared_ptr<Reticle> m_reticle;

	bool isCollisionDebugMode = false;

	float setSpeed = 10.0f;

	float setAimDistance = 2000.0f;

	Vector3 setRot = { 0,0,0 };

	int enemyCount = 0;

	std::shared_ptr<PlayAreaComponent> m_playArea;

	std::shared_ptr<MoveComponent> m_playerMove;
};
