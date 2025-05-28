#pragma once
#include "IScene.h"

class TitleScene : public IScene
{
public:
	explicit TitleScene() {};
	//~TestScene() {};
	void Update(uint64_t deltatime) override;
	void Draw(uint64_t deltatime) override;
	void Init() override;
	void Uninit() override;
private:
};
