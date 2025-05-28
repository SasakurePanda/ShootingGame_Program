#pragma once
#include "IScene.h"

class TestScene : public IScene
{
public:
	explicit TestScene() {};
	//~TestScene() {};
	void Update(uint64_t deltatime) override;
	void Draw(uint64_t deltatime) override;
	void Init() override;
	void Uninit() override;
private:
};