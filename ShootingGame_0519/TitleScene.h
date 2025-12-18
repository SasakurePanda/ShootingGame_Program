#pragma once
#include <vector>
#include <d3d11.h>
#include "IScene.h"
#include "TitleBackGround.h"
#include "SkyDome.h"

//---------------------------------
//IScene‚ğŒp³‚µ‚½GameScene
//---------------------------------
class TitleScene : public IScene
{
public:
	explicit TitleScene() {};
	void Update(float deltatime) override;
	void Draw(float deltatime) override;
	//3Dƒ[ƒ‹ƒhã‚Ì•`‰æŠÖ”
	void DrawWorld(float deltatime) override;
	//UIã‚Ì•`‰æŠÖ”
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
};



