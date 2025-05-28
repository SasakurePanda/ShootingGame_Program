#pragma once

class Game
{
public:
	static void GameInit();			//初期化
	static void GameUpdate(uint64_t deltaTime);		//更新
	static void GameDraw(uint64_t deltaTime);		//描画
	static void GameUninit();		//終了処理
	static void GameLoop();		//ゲームのメインループ
};