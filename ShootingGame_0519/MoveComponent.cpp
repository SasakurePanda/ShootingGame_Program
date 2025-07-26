#include "MoveComponent.h"
#include "GameObject.h"

using namespace DirectX::SimpleMath;

void MoveComponent::Update()
{
    // 入力取得
    SRT move = { {0,0,0} ,{0,0,0} ,{0,0,0} };

    if (Input::IsKeyDown('W'))
    {
        move.pos.z += 1.0f;
    }
    if (Input::IsKeyDown('S')) 
    {
        move.pos.z -= 1.0f;
    }
    if (Input::IsKeyDown('A')) 
    {
        move.pos.x += 1.0f;
    }
    if (Input::IsKeyDown('D'))
    {
        move.pos.x -= 1.0f;
    }

    if (move.pos.LengthSquared() > 0.0f)
    {
        move.pos.Normalize();
        move.pos *= m_speed * 1.0f / 60.0f; // 仮に60FPS前提で1フレーム分の移動

        // 現在の位置を取得・更新
        Vector3 pos = GetOwner()->GetPosition();
        pos += move.pos;
        GetOwner()->SetPosition(pos);
    }
}
