#pragma once
#include "commontypes.h"
#include <memory>

class ColliderComponent;
class GameObject;

namespace CollisionResponse
{
    // 衝突の押し出しを解決する。戻り値: true = 衝突が検出され押し出しを行った
    bool ResolvePenetration(ColliderComponent* a, ColliderComponent* b);

    // 指定オブジェクトにノックバック（インパルス）を与える。direction は「押し出し方向 (world)」。
    // magnitude は力の大きさ（単位はゲーム内任意）。内部で MoveComponent を探して AddImpulse を呼ぶ。
    void ApplyKnockback(GameObject* target, const DirectX::SimpleMath::Vector3& direction, float magnitude);

    // 汎用：GameObject にインパルスを適用（MoveComponent が無ければ false を返す）
    bool TryApplyImpulseToObject(GameObject* target, const DirectX::SimpleMath::Vector3& impulse);
}

