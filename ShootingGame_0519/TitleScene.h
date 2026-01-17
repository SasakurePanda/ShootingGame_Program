#pragma once
#include <vector>
#include <d3d11.h>
#include "IScene.h"
#include "TitleBackGround.h"
#include "TitlePlayerMotionComponent.h"
#include "CameraObject.h"
#include "SkyDome.h"

//---------------------------------
//ISceneを継承したGameScene
//---------------------------------
class TitleScene : public IScene
{
public:
	explicit TitleScene() {};
	void Update(float deltatime) override;
	void Draw(float deltatime) override;
	//3Dワールド上の描画関数
	void DrawWorld(float deltatime) override;
	//UI上の描画関数
	void DrawUI(float deltatime) override;
	void Init() override;
	void Uninit() override;
	void AddObject(std::shared_ptr<GameObject> obj) override;
	void AddTextureObject(std::shared_ptr<GameObject> obj);
	void RemoveObject(std::shared_ptr<GameObject>) override;
	void RemoveObject(GameObject* obj);
	void FinishFrameCleanup() override;
	const std::vector<std::shared_ptr<GameObject>>& GetObjects() const override { return m_GameObjects; }
private:

	void SetSceneObject();
	std::vector<std::shared_ptr<GameObject>> m_GameObjects;
	std::vector<std::shared_ptr<GameObject>> m_TextureObjects;
	std::vector<std::shared_ptr<GameObject>> m_DeleteObjects;
	std::vector<std::shared_ptr<GameObject>> m_AddObjects;
	
	std::shared_ptr<SkyDome> m_SkyDome;
	std::shared_ptr<GameObject> m_Player;
	std::shared_ptr<CameraObject> m_camera;

	std::shared_ptr<GameObject> m_TitleLogo;	//タイトルロゴオブジェクト
	std::shared_ptr<GameObject> m_TitleText;	//タイトルテキストオブジェクト

	std::shared_ptr<TitlePlayerMotionComponent> m_TitleMotion;
	bool m_IsLogoShown = false;				//2Dロゴが表示されたかどうか

	//--------------ロゴ点滅関連------------------
	float m_BlinkTimer = 0.0f;
	float m_BlinkInterval = 0.5f;
	bool m_BlinkVisible = true;

	std::vector<BezierPath> m_PlayerPaths;
	int m_CurrentPathIndex = 0;
	bool m_LoopPaths = true;

	void SetupPlayerPaths();
	void ApplyCurrentPlayerPath();
	void AdvancePlayerPath();
};



