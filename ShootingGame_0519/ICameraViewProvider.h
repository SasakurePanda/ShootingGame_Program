#pragma once
#include "commontypes.h"
//----------------------------------------------------
//各カメラクラスに前方向と右方向ベクトルを
//取得できる関数を持たせるためのインターフェイスクラス
//----------------------------------------------------
class ICameraViewProvider
{
public:
    virtual Vector3 GetForward() const = 0;
    virtual Vector3 GetRight() const = 0;
};
