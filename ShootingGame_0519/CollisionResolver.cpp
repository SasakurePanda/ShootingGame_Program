#include "CollisionResolver.h"
#include "CollisionHelpers.h"
#include "AABBColliderComponent.h"
#include "OBBColliderComponent.h"

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <limits>

using namespace DirectX::SimpleMath;

namespace Collision
{
    static const float EPS = 1e-6f;

    // （互換用スタブ。プロジェクトで使わないなら削除しても良い）
    bool ComputeAABBvsOBBMTV(
        const Vector3* /*aabbMinPtr*/,
        const Vector3* /*aabbMaxPtr*/,
        const void* /*unused*/,
        Vector3& /*outPushForA*/,
        Vector3& /*outPushForB*/)
    {
        return false;
    }

    bool ComputeAABBvsOBBMTV(
        const AABBColliderComponent* aabb,
        const OBBColliderComponent* obb,
        Vector3& outPushForA,
        Vector3& outPushForB)
    {
        if (!aabb || !obb)
        {
            return false;
        }

        // AABB の中心と半幅（ワールド）
        Vector3 aMin = aabb->GetMin();
        Vector3 aMax = aabb->GetMax();
        Vector3 aCenterV = (aMin + aMax) * 0.5f;
        Vector3 aHalfV = (aMax - aMin) * 0.5f;

        // OBB の中心と半幅、軸（ワールド）
        Vector3 bCenter = obb->GetCenter();
        Vector3 bHalfV = obb->GetSize() * 0.5f;
        Matrix rotB = obb->GetRotationMatrix();

        // World axes for A (AABB) are just the world basis
        Vector3 axesA[3] = { Vector3(1,0,0), Vector3(0,1,0), Vector3(0,0,1) };

        Vector3 axesB[3];
        ExtractAxesFromRotation(rotB, axesB); // 正規化された Right, Up, Forward

        // t = centerB - centerA (world)
        Vector3 tWorld = bCenter - aCenterV;

        // R[i][j] = A_i dot B_j
        float R[3][3];
        float AbsR[3][3];
        for (int i = 0; i < 3; ++i)
        {
            for (int j = 0; j < 3; ++j)
            {
                R[i][j] = axesA[i].Dot(axesB[j]);
                AbsR[i][j] = std::fabs(R[i][j]) + 1e-6f; // small epsilon
            }
        }

        // t の A 軸成分
        float tA[3] = { tWorld.Dot(axesA[0]), tWorld.Dot(axesA[1]), tWorld.Dot(axesA[2]) };

        // float 配列に展開して添字アクセスを可能にする
        float aHalf[3] = { aHalfV.x, aHalfV.y, aHalfV.z };
        float bHalf[3] = { bHalfV.x, bHalfV.y, bHalfV.z };

        // 最小のオーバーラップを探す
        float minOverlap = FLT_MAX;
        Vector3 minAxis = Vector3::Zero;
        bool found = false;

        auto testAxis = [&](const Vector3& axis, float overlap, const Vector3& axisDirCandidate)
            {
                if (overlap <= 0.0f)
                {
                    found = false;
                    return;
                }

                if (overlap < minOverlap)
                {
                    minOverlap = overlap;
                    minAxis = axisDirCandidate;
                    found = true;
                }
            };

        // --- A の軸チェック (world X,Y,Z) ---
        for (int i = 0; i < 3; ++i)
        {
            float ra = aHalf[i];
            float rb = bHalf[0] * AbsR[i][0] + bHalf[1] * AbsR[i][1] + bHalf[2] * AbsR[i][2];
            float overlap = ra + rb - std::fabs(tA[i]);
            Vector3 axis = axesA[i];
            float sign = (tA[i] >= 0.0f) ? 1.0f : -1.0f;
            Vector3 dir = axis * sign;
            testAxis(axis, overlap, dir);
            if (!found && overlap <= 0.0f)
            {
                return false;
            }
        }

        // --- B の軸チェック ---
        for (int i = 0; i < 3; ++i)
        {
            float ra = aHalf[0] * AbsR[0][i] + aHalf[1] * AbsR[1][i] + aHalf[2] * AbsR[2][i];
            float rb = bHalf[i];
            float tProj = std::fabs(tA[0] * R[0][i] + tA[1] * R[1][i] + tA[2] * R[2][i]);
            float overlap = ra + rb - tProj;

            float projVal = (tA[0] * R[0][i] + tA[1] * R[1][i] + tA[2] * R[2][i]);
            float projSign = (projVal >= 0.0f) ? 1.0f : -1.0f;
            Vector3 dir = axesB[i] * projSign;

            testAxis(axesB[i], overlap, dir);
            if (!found && overlap <= 0.0f)
            {
                return false;
            }
        }

        // --- A_i x B_j チェック (9個) ---
        for (int i = 0; i < 3; ++i)
        {
            for (int j = 0; j < 3; ++j)
            {
                // axis = Ai x Bj
                Vector3 axis = axesA[i].Cross(axesB[j]);
                float axisLenSq = axis.LengthSquared();
                if (axisLenSq < 1e-8f)
                {
                    // nearly parallel axes -> skip
                    continue;
                }
                axis.Normalize();

                int i1 = (i + 1) % 3;
                int i2 = (i + 2) % 3;
                int j1 = (j + 1) % 3;
                int j2 = (j + 2) % 3;

                float ra = aHalf[i1] * AbsR[i2][j] + aHalf[i2] * AbsR[i1][j];
                float rb = bHalf[j1] * AbsR[i][j2] + bHalf[j2] * AbsR[i][j1];

                float tProj = std::fabs(tA[i2] * R[i1][j] - tA[i1] * R[i2][j]);
                float overlap = ra + rb - tProj;

                float s = tWorld.Dot(axis);
                float sign = (s >= 0.0f) ? 1.0f : -1.0f;
                Vector3 dir = axis * sign;

                testAxis(axis, overlap, dir);
                if (!found && overlap <= 0.0f)
                {
                    return false;
                }
            }
        }

        if (!found)
        {
            return false;
        }

        Vector3 mtv = minAxis;

        if (mtv.LengthSquared() > EPS)
        {
            mtv.Normalize();
        }
        else
        {
            mtv = Vector3::Zero;
        }

        Vector3 push = mtv * minOverlap;

        outPushForA = push;
        outPushForB = -push;

        return true;
    }

    bool ComputeAABBMTV(const AABBColliderComponent* a, const AABBColliderComponent* b,
        Vector3& outPushA, Vector3& outPushB)
    {
        if (!a || !b) return false;

        Vector3 aMin = a->GetMin();
        Vector3 aMax = a->GetMax();
        Vector3 bMin = b->GetMin();
        Vector3 bMax = b->GetMax();

        // overlaps along each axis
        float overlapX = min(aMax.x, bMax.x) - max(aMin.x, bMin.x);
        float overlapY = min(aMax.y, bMax.y) - max(aMin.y, bMin.y);
        float overlapZ = min(aMax.z, bMax.z) - max(aMin.z, bMin.z);

        if (overlapX <= 0.0f || overlapY <= 0.0f || overlapZ <= 0.0f)
        {
            return false; // no overlap
        }

        // choose axis of minimum penetration
        float minOverlap = overlapX;
        int axis = 0; // 0=x,1=y,2=z
        if (overlapY < minOverlap) { minOverlap = overlapY; axis = 1; }
        if (overlapZ < minOverlap) { minOverlap = overlapZ; axis = 2; }

        // compute sign based on center difference
        Vector3 centerA = (aMin + aMax) * 0.5f;
        Vector3 centerB = (bMin + bMax) * 0.5f;
        Vector3 dir = centerA - centerB; // direction from B to A

        // ensure non-zero
        if (dir.LengthSquared() < 1e-6f)
        {
            dir = Vector3(0, 1, 0); // fallback
        }

        Vector3 push = Vector3::Zero;
        switch (axis)
        {
        case 0: // x
            push.x = (dir.x >= 0.0f) ? minOverlap : -minOverlap;
            break;
        case 1: // y
            push.y = (dir.y >= 0.0f) ? minOverlap : -minOverlap;
            break;
     
        case 2: // z
            push.z = (dir.z >= 0.0f) ? minOverlap : -minOverlap;
            break;
        }

        outPushA = push;      // move A by push
        outPushB = -push;     // move B opposite
        return true;
    }

}
