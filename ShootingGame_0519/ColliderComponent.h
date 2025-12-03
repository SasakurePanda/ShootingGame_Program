#pragma once
#include "Component.h"
#include "commontypes.h"

//コライダーの種類
enum ColliderType
{
    AABB,
    OBB,
};

//---------------------------------------------------------------
//  コライダーコンポーネント基底クラス
//  このクラス自体は当たり判定のデータを定義するだけで
//  当たり判定の計算は別ファイルで行っています
//---------------------------------------------------------------

class ColliderComponent : public Component
{
public:
    ColliderComponent(ColliderType type): m_Type(type) {}
    virtual ~ColliderComponent() override;

    //共通のGet/Set関数
    ColliderType GetColliderType() const { return m_Type; }               //コライダーのType(AABB/OBB)のゲット関数

    virtual Vector3 GetCenter() const = 0;     //中心の座標ゲット関数
    virtual Vector3 GetSize()   const = 0;     //当たり判定のサイズゲット関数
    virtual DirectX::SimpleMath::Matrix GetRotationMatrix() const = 0;    //回転行列のゲット関数
    
    //当たり判定の有効/無効のセット関数
    void SetEnabled(bool enabled);
    
    //当たり判定が有効か無効かを返すゲット関数
    bool IsEnabled() const { return m_enabled; };              

    void SetHitThisFrame(bool hit) { m_hitThisFrame = hit; }
    bool IsHitThisFrame() const { return m_hitThisFrame; }

protected:
    ColliderType m_Type;
    bool m_hitThisFrame = false; //毎フレームの衝突状態
	bool m_enabled = true;       //当たり判定の有効/無効
};
