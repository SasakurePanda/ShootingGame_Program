#pragma once
#include "GameObject.h"
//---------------------------------------------------------
// PlayerやEnemyの一動作を制作するうえで規定となるクラス
// 継承して使う(例）ジャンプ、ダッシュ、防御など)
//---------------------------------------------------------

class GameObject; // ← これを追加！

class Component
{
public:
    virtual ~Component() = default; //仮想デストラクタ

    virtual void Initialize() {} //ゲームループ内のInit
    virtual void Update(float dt) {};   //ゲームループ内のUpdate
    virtual void Draw(float alpha) {};  //ゲームループ内のDraw

    //コンポーネントが属するクラス(GameObjectなど)を参照、保持する仕組み
    //例）コンポーネント内での更新で親オブジェクトの位置や
    //　　速度を変更したい場合にm_owerを使う
    void SetOwner(GameObject* owner)
    {
        m_owner = owner;
    }

    GameObject* GetOwner() const
    {
        return m_owner;
    }

protected:
    //属しているGameObjectのポインタを保持する変数
    GameObject* m_owner = nullptr;
};
