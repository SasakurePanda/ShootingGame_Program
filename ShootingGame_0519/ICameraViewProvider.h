#pragma once

#include "commontypes.h"    
#include <SimpleMath.h>    

// 型を明示するために名前空間で短縮
namespace SMS = DirectX::SimpleMath;

class ICameraViewProvider
{
public:
    virtual ~ICameraViewProvider() = default;

    // 方向ベクトル（前・右）
    virtual SMS::Vector3 GetForward() const = 0;
    virtual SMS::Vector3 GetRight() const = 0;

    // 現在のマウスが指すワールド座標（MoveComponent が問い合わせる）
    virtual SMS::Vector3 GetAimPoint() const = 0;

    // ビュー／プロジェクション行列
    virtual SMS::Matrix GetView() const = 0;
    virtual SMS::Matrix GetProj() const = 0;

    // ワールド座標（カメラ位置）
    virtual SMS::Vector3 GetPosition() const = 0;

    virtual SMS::Vector3 GetAimDirectionFromReticle() const = 0;

    // ブースト状態を外部から通知する（既存の実装を壊さないためデフォルトは no-op）
    virtual void SetBoostState(bool isBoosting) { /* default: ignore */ }

    virtual SMS::Vector2 GetReticleScreen() const = 0;

};

