#pragma once
#include <cstdint>
//------------------------ISceneクラス-------------------------
//Sceneを作る際などに使うインターフェースとしてのクラス
//-------------------------------------------------------------
class IScene
{
public:
	
	//何もしないけど必須なコンストラクタ
	IScene() = default;			
	
	//何もしないけど必須なデストラクタ
	virtual ~IScene() = default; 

	//-------------一連の流れに関しての純粋仮想関数-------------
	//Update関数(uint64_t deltaは前回からの経過時間)
	virtual void Update(uint64_t delta) = 0; 

	//Draw関数(uint64_t deltaは前回からの経過時間)
	virtual void Draw(uint64_t delta) = 0;	 

	//Init関数
	virtual void Init() = 0;				 

	//Uninit関数
	virtual void Uninit() = 0;	
	//-----------------------------------------------------------
};
