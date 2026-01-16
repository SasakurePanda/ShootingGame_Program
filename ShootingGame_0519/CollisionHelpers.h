#pragma once
#include <SimpleMath.h>
#include <algorithm> 

#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

//----------------------------------------------------------------
// 当たり判定を取る際に使えるような、数学系の処理をまとめておく
// ヘッダーファイルです
//----------------------------------------------------------------
using namespace DirectX::SimpleMath;

inline void ExtractAxesFromRotation(const Matrix& rot, Vector3 outAxes[3])
{
    outAxes[0] = Vector3(rot._11, rot._12, rot._13); // Right
    outAxes[1] = Vector3(rot._21, rot._22, rot._23); // Up
    outAxes[2] = Vector3(rot._31, rot._32, rot._33); // Forward

    outAxes[0].Normalize();
    outAxes[1].Normalize();
    outAxes[2].Normalize();
}

inline Vector3 ClosestPtPointOBB(
    const Vector3& point,
    const Vector3& obbCenter,
    const Vector3 obbAxes[3],
    const Vector3& obbHalfSize)
{
    Vector3 d = point - obbCenter;
    Vector3 q = obbCenter;

    float dist = 0.0f;

    dist = d.Dot(obbAxes[0]);
    dist = std::max(-obbHalfSize.x, std::min(dist, obbHalfSize.x));
    q += obbAxes[0] * dist;

    dist = d.Dot(obbAxes[1]);
    dist = std::max(-obbHalfSize.y, std::min(dist, obbHalfSize.y));
    q += obbAxes[1] * dist;

    dist = d.Dot(obbAxes[2]);
    dist = std::max(-obbHalfSize.z, std::min(dist, obbHalfSize.z));
    q += obbAxes[2] * dist;

    return q;
}

