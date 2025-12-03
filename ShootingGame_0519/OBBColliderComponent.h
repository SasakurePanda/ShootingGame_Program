#pragma once
#include <DirectXMath.h>
#include <SimpleMath.h>
#include "ColliderComponent.h"

//--------------------------------------------------------------------------------
//  OBBによる当たり判定(軸に対して任意の角度で回転できる当たり判定)のクラス
//  回転も考えた当たり判定、難しい
//  OBBはSATで判定するため、判定には中心・3軸（Right/Up/Forward）・半サイズが必要
//--------------------------------------------------------------------------------
class OBBColliderComponent : public ColliderComponent
{
public:
    //コンストラクタ(当たり判定のタイプがOBBと決めている)
    OBBColliderComponent() : ColliderComponent(ColliderType::OBB) {}

    //当たり判定の大きさを指定する関数
    //(全体の幅・高さ・奥行き)
    void SetSize(const Vector3& size);

    //GameObjectの位置のゲット関数
    Vector3 GetCenter() const override;

    //当たり判定用のサイズのゲット関数(m_Size)
    Vector3 GetSize() const override;

    //ゲームオブジェクトの回転値から、回転行列を生成。
    DirectX::SimpleMath::Matrix GetRotationMatrix() const override;

    void SetLocalOffset(const Vector3& offset) { m_LocalOffset = offset; }
    const Vector3& GetLocalOffset() const { return m_LocalOffset; }

private:

    //幅、高さ、奥行の大きさをそれぞれ設定できる変数
    Vector3 m_Size = Vector3(1, 1, 1);

    DirectX::SimpleMath::Vector3 m_LocalOffset = DirectX::SimpleMath::Vector3::Zero;
};

