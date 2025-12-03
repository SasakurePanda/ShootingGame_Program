#include "AABBColliderComponent.h"
#include "GameObject.h"

//------------------------------------------------
// GameObjectの位置を中心にローカルオフセットを
// 加えてからCenterを返す
//------------------------------------------------
Vector3 AABBColliderComponent::GetCenter() const
{
    GameObject* owner = GetOwner();
    if (!owner)
    {
        return Vector3::Zero;
    }

    //ローカルオフセット * オブジェクトのサイズ
    Vector3 scaledOffset = Vector3(
        m_LocalOffset.x * owner->GetScale().x,
        m_LocalOffset.y * owner->GetScale().y,
        m_LocalOffset.z * owner->GetScale().z
    );

	//オブジェクトの位置にオフセットを足して返す
    return owner->GetPosition() + scaledOffset;
}

//------------------------------------------------
// 当たり判定用のサイズ
// (ワールドスケールを反映したサイズ)を返す
//------------------------------------------------
Vector3 AABBColliderComponent::GetSize() const
{
    GameObject* owner = GetOwner();
    if (!owner) 
    {
        //フォールバックはローカルサイズ
        return Vector3::Zero;
    }

    Vector3 s = owner->GetScale();

	//オブジェクトの大きさを反映したサイズを返す
    return Vector3(
        m_Size.x * s.x,
        m_Size.y * s.y,
        m_Size.z * s.z);
}

//------------------------------------------------
// AABBは回転しないが、
// GetRotationMatrixは互換性のために置いておく
//------------------------------------------------
DirectX::SimpleMath::Matrix AABBColliderComponent::GetRotationMatrix() const
{
	//AABBは回転しないのでIdentityを返す
    return DirectX::SimpleMath::Matrix::Identity;
}

//------------------------------------------------
// 中心とサイズから
// min(一番小さい値を持つ点（左下奥の角)と
// max(一番大きい値を持つ点（右上手前の角)を
// 計算して返す
//------------------------------------------------
Vector3 AABBColliderComponent::GetMin() const
{   
    
    //コライダーのワールド空間での中心位置を取得
    Vector3 center = GetCenter();

	//スケールを考慮したサイズを取得
    Vector3 size = GetSize();

	//中心から半分のサイズを引いて最小座標を出す
    return center - size * 0.5f;
}

Vector3 AABBColliderComponent::GetMax() const
{
    //コライダーのワールド空間での中心位置を取得
    Vector3 center = GetCenter();

    //スケールを考慮したサイズを取得
    Vector3 size = GetSize();

	//中心から半分のサイズを引いて最大座標を出す
    return center + size * 0.5f;
}
