#include "AABBColliderComponent.h"
#include "GameObject.h"

//GameObjectの位置のゲット関数
Vector3 AABBColliderComponent::GetCenter() const
{
    GameObject* owner = GetOwner();
    if (!owner)
    {
        return Vector3::Zero;
    }
    return owner->GetPosition();
}

//当たり判定用のサイズのゲット関数(m_Size)
Vector3 AABBColliderComponent::GetSize() const
{
    GameObject* owner = GetOwner();
    if (!owner) return Vector3::Zero;

    // ローカルオフセットをワールドスケールで伸ばして足す（回転はAABBでは無視）
    Vector3 scaledOffset = Vector3(m_LocalOffset.x * owner->GetScale().x,
        m_LocalOffset.y * owner->GetScale().y,
        m_LocalOffset.z * owner->GetScale().z);
    return owner->GetPosition() + scaledOffset;
}

//AABBは回転を考慮しないのでIdentity(回転なし)にしておく
DirectX::SimpleMath::Matrix AABBColliderComponent::GetRotationMatrix() const
{
    GameObject* owner = GetOwner();
    if (!owner)
    {
        return DirectX::SimpleMath::Matrix::Identity;
    }
    Vector3 rot = owner->GetRotation();
    return DirectX::SimpleMath::Matrix::CreateFromYawPitchRoll(rot.y, rot.x, rot.z);
}

// 修正後（GetSize() を使ってワールドスケールを反映）
Vector3 AABBColliderComponent::GetMin() const
{
    return GetCenter() - GetSize() * 0.5f;
}

Vector3 AABBColliderComponent::GetMax() const
{
    return GetCenter() + GetSize() * 0.5f;
}
