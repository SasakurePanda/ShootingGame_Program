#pragma once
#include <cstdint>
#include "SceneManager.h"
#include "RaycastHit.h"
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
	virtual void Update(float delta) = 0; 

	//Draw関数(uint64_t deltaは前回からの経過時間)
	virtual void Draw(float delta) = 0;	 

	//Init関数
	virtual void Init() = 0;				 

	//Uninit関数
	virtual void Uninit() = 0;	
	//---------------他からオブジェクトの追加を行う関数--------------
	virtual void AddObject(std::shared_ptr<class GameObject> obj) = 0;

	//------------他からオブジェクトの削除の登録を行う関数---------------
	virtual void RemoveObject(std::shared_ptr<GameObject> obj) = 0;

	virtual void RemoveObject(GameObject* obj) = 0;
	//---------------他からオブジェクトの削除を行う関数------------------
	virtual void FinishFrameCleanup() = 0;
	//-----------------Raycastで当たった物を取ってくる関数------------------
	virtual bool Raycast(const DirectX::SimpleMath::Vector3& origin,
		const DirectX::SimpleMath::Vector3& dir,
		float maxDistance,
		RaycastHit& outHit,
		GameObject* ignore = nullptr) { return false; };
	
	//---------------シーン内にあるオブジェクトを持ってくる関数------------------
	virtual const std::vector<std::shared_ptr<GameObject>>& GetObjects() const = 0;
};
