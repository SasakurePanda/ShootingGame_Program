#pragma once
#include <cmath>
#include <SimpleMath.h>
#include "AABBColliderComponent.h"
#include "OBBColliderComponent.h"
#include "CollisionHelpers.h" 

using namespace DirectX::SimpleMath;

namespace Collision
{
    // ヘルパ：Matrix からローカル軸 (Right, Up, Forward) を抽出して正規化する
    inline void ExtractAxesFromMatrix(const Matrix& m, Vector3 outAxes[3])
    {
        // SimpleMath::Matrix は要素 _11.._44 を持つ（行列表現に依存するが、ここでは基底ベクトルを列として読み取る）
        // 出力は Right, Up, Forward の順
        outAxes[0] = Vector3(m._11, m._12, m._13); // Right
        outAxes[1] = Vector3(m._21, m._22, m._23); // Up
        outAxes[2] = Vector3(m._31, m._32, m._33); // Forward

        // 正規化しておく（回転行列なら単位長だが誤差対策）
        outAxes[0].Normalize();
        outAxes[1].Normalize();
        outAxes[2].Normalize();
    }

    // ---------------------------------------
    // AABB vs AABB （直接の数値）
    // ---------------------------------------
    inline bool IsAABBHit(
        const Vector3& minA, const Vector3& maxA,
        const Vector3& minB, const Vector3& maxB)
    {
        //x
        const bool xOverlap = (minA.x <= maxB.x) && (maxA.x >= minB.x);
        const bool yOverlap = (minA.y <= maxB.y) && (maxA.y >= minB.y);
        const bool zOverlap = (minA.z <= maxB.z) && (maxA.z >= minB.z);

        return xOverlap && yOverlap && zOverlap;
    }


    // ---------------------------------------
    // AABB vs AABB （コンポーネント版）
    // ---------------------------------------
    inline bool IsAABBHit(
        const AABBColliderComponent* a, 
        const AABBColliderComponent* b)
    {
        const Vector3 aMin = a->GetMin();
        const Vector3 aMax = a->GetMax();
        const Vector3 bMin = b->GetMin();
        const Vector3 bMax = b->GetMax();

        const bool xOverlap = (aMin.x <= bMax.x) && (aMax.x >= bMin.x);
        const bool yOverlap = (aMin.y <= bMax.y) && (aMax.y >= bMin.y);
        const bool zOverlap = (aMin.z <= bMax.z) && (aMax.z >= bMin.z);

        return xOverlap && yOverlap && zOverlap;
    }

    // ----------------------------------------------------
    // OBB vs OBB（SAT）生データ版（中心/軸/半サイズで判定）
    // ----------------------------------------------------
    inline bool IsOBBHit(
        const Vector3& centerA, const Vector3* axesA, const Vector3& halfSizeA,
        const Vector3& centerB, const Vector3* axesB, const Vector3& halfSizeB)
    {
        const float EPSILON = 1e-6f;

        // すでに axesA, axesB が渡されているので Extract は不要

        // 中心間ベクトル
        Vector3 tWorld = centerB - centerA;

        // R 行列 = Ai dot Bj
        float R[3][3], AbsR[3][3];
        for (int i = 0; i < 3; ++i)
        {
            for (int j = 0; j < 3; ++j)
            {
                R[i][j] = axesA[i].Dot(axesB[j]);
                AbsR[i][j] = std::fabs(R[i][j]) + EPSILON;
            }
        }

        // t を A の軸に沿った成分
        float tA[3] = { tWorld.Dot(axesA[0]), tWorld.Dot(axesA[1]), tWorld.Dot(axesA[2]) };

        float aHalf[3] = { halfSizeA.x, halfSizeA.y, halfSizeA.z };
        float bHalf[3] = { halfSizeB.x, halfSizeB.y, halfSizeB.z };

        float ra, rb;

        // --- A の軸チェック ---
        for (int i = 0; i < 3; ++i)
        {
            ra = aHalf[i];
            rb = bHalf[0] * AbsR[i][0] + bHalf[1] * AbsR[i][1] + bHalf[2] * AbsR[i][2];
            if (std::fabs(tA[i]) > ra + rb) return false;
        }

        // --- B の軸チェック ---
        for (int i = 0; i < 3; ++i)
        {
            ra = aHalf[0] * AbsR[0][i] + aHalf[1] * AbsR[1][i] + aHalf[2] * AbsR[2][i];
            rb = bHalf[i];
            float tProj = std::fabs(tA[0] * R[0][i] + tA[1] * R[1][i] + tA[2] * R[2][i]);
            if (tProj > ra + rb) return false;
        }

        // --- 交差軸 Ai x Bj ---
        for (int i = 0; i < 3; ++i)
        {
            for (int j = 0; j < 3; ++j)
            {
                int i1 = (i + 1) % 3;
                int i2 = (i + 2) % 3;
                int j1 = (j + 1) % 3;
                int j2 = (j + 2) % 3;

                ra = aHalf[i1] * AbsR[i2][j] + aHalf[i2] * AbsR[i1][j];
                rb = bHalf[j1] * AbsR[i][j2] + bHalf[j2] * AbsR[i][j1];

                float tProj = std::fabs(tA[i2] * R[i1][j] - tA[i1] * R[i2][j]);
                if (tProj > ra + rb) return false;
            }
        }

        return true;
    }


    inline bool IsOBBHit(
        const OBBColliderComponent* a,
        const OBBColliderComponent* b)
    {
        const Vector3 centerA = a->GetCenter();
        const Matrix  rotA = a->GetRotationMatrix();
        const Vector3 halfA = a->GetSize() * 0.5f;

        const Vector3 centerB = b->GetCenter();
        const Matrix  rotB = b->GetRotationMatrix();
        const Vector3 halfB = b->GetSize() * 0.5f;

        // rotA / rotB から軸を抽出
        Vector3 axesA[3];
        Vector3 axesB[3];
        ExtractAxesFromMatrix(rotA, axesA);
        ExtractAxesFromMatrix(rotB, axesB);

        // 下の関数に軸ごと渡して判定
        return IsOBBHit(centerA, axesA, halfA, centerB, axesB, halfB);
    }

    // ---------------------------------------------
    // AABB vs OBB（コンポーネント版）
    // AABB を「回転なしの OBB」として流用
    // ---------------------------------------------
    inline bool IsAABBvsOBBHit(
        const AABBColliderComponent* aabb, 
        const OBBColliderComponent* obb)
    {
        const Vector3 aMin = aabb->GetMin();
        const Vector3 aMax = aabb->GetMax();
        const Vector3 centerA = (aMin + aMax) * 0.5f;
        const Vector3 halfA   = (aMax - aMin) * 0.5f;

        Vector3 axesA[3] = 
        {
            Vector3(1,0,0), //world X
            Vector3(0,1,0), //world Y
            Vector3(0,0,1)  //world Z
        };

        const Vector3 centerB = obb->GetCenter();
        const Vector3 halfB   = obb->GetSize() * 0.5f;

        Vector3 axesB[3];
        ExtractAxesFromRotation(obb->GetRotationMatrix(), axesB);

        return IsOBBHit(centerA, axesA, halfA, centerB, axesB, halfB);
    }

    inline bool IsAABBvsOBBHit(
        const Vector3& aabbMin,
        const Vector3& aabbMax,
        const Vector3& obbCenter,
        const Matrix&  obbRot,
        const Vector3& obbHalfSize)
    {
        Vector3 aabbCenter = (aabbMin + aabbMax) * 0.5f;
        Vector3 aabbHalfSize = (aabbMax - aabbMin) * 0.5f;

        Vector3 axesA[3] = {
            Vector3(1,0,0),
            Vector3(0,1,0),
            Vector3(0,0,1)
        };

        Vector3 axesB[3];
        ExtractAxesFromMatrix(obbRot, axesB); // Ensure this function exists and normalizes

        return IsOBBHit(aabbCenter, axesA, aabbHalfSize, obbCenter, axesB, obbHalfSize);
    }

    
}
