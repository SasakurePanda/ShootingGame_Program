#include "Building.h"

void Building::Initialize()
{
    //基底クラスの初期化（Component群）
    GameObject::Initialize();
}

void Building::Update(float dt)
{
    //コンポーネント等のUpdate
    GameObject::Update(dt);

}