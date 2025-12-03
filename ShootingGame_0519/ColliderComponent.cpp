#include "ColliderComponent.h"
#include "CollisionManager.h"

ColliderComponent::~ColliderComponent()
{
    // オブジェクト破棄時に登録解除（CollisionManager の宣言が必要なので cpp に実装）
    CollisionManager::UnregisterCollider(this);
}

void ColliderComponent::SetEnabled(bool enabled)
{
    m_enabled = enabled;
    if (!m_enabled)
    {
        CollisionManager::UnregisterCollider(this);
    }
}