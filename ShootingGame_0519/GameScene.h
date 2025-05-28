#pragma once
#include "IScene.h"
//---------------------------------
//IScene‚ğŒp³‚µ‚½GameScene
//---------------------------------
class GameScene : public IScene
{
	explicit GameScene() {};
	void Update(uint64_t deltatime) override;
	void Draw(uint64_t deltatime) override;
	void Init() override;
	void Uninit() override;
};