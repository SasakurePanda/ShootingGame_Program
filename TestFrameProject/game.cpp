#include	"system/renderer.h"
#include	"fpscontrol.h"

void gameinit() 
{
	// レンダラの初期化
	Renderer::Init();

}

void gameupdate(uint64_t deltatime)
{

}

void gamedraw(uint64_t deltatime) 
{
	// レンダリング前処理
	Renderer::Begin();

	// レンダリング後処理
	Renderer::End();

}

void gamedispose() 
{
	// レンダラの終了処理
	Renderer::Uninit();

}

void gameloop()
{
	uint64_t delta_time = 0;

	// フレームの待ち時間を計算する
	static FPS fpsrate(70);

	// 前回実行されてからの経過時間を計測
	delta_time = fpsrate.CalcDelta();

	// 更新処理、描画処理を呼び出す
	gameupdate(delta_time);
	gamedraw(delta_time);

	// 規定時間までWAIT
	fpsrate.Wait();
}