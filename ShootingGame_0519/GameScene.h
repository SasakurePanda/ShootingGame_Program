#pragma once
#include <vector>
#include <d3d11.h>
#include "IScene.h"
#include "FreeCamera.h"
#include "Player.h"
#include "CameraObject.h"
#include "GridFloor.h"

//---------------------------------
//ISceneÇåpè≥ÇµÇΩGameScene
//---------------------------------
class GameScene : public IScene
{
public:
	explicit GameScene() {};
	void Update(uint64_t deltatime) override;
	void Draw(uint64_t deltatime) override;
	void Init() override;
	void Uninit() override;
	void AddObject(std::shared_ptr<GameObject> obj) override;

private:
	std::shared_ptr<Player> m_player;
	std::shared_ptr<CameraObject> m_FollowCamera;
	std::vector<std::shared_ptr<GameObject>> m_GameObjects;
	std::shared_ptr<GridFloor> m_GridFloor;
};
