#pragma once
#include <SimpleMath.h>
/// <summary>
/// 押し出し用に共通のスピード感利用インターフェース
/// </summary>
class IMovable
{
public:
    virtual DirectX::SimpleMath::Vector3 GetVelocity() const = 0;
    virtual void SetVelocity(const DirectX::SimpleMath::Vector3& v) = 0;
    virtual ~IMovable() = default;
};