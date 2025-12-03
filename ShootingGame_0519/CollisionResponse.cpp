#include "CollisionResponse.h"
#include "ColliderComponent.h"
#include "AABBColliderComponent.h"
#include "OBBColliderComponent.h"
#include "CollisionResolver.h" // 既存の ComputeAABBvsOBBMTV / ComputeAABBMTV を利用
#include "MoveComponent.h"
#include "GameObject.h"
#include <algorithm>
#include <cmath>
#include <iostream>

using namespace DirectX::SimpleMath;

namespace CollisionResponse
{
    // OBB vs OBB の MTV を計算するヘルパ（SATを使って最小分離軸を探す）
    // 戻り: true=交差している（outPushForA/outPushForB に押し出しベクトル）
    static bool ComputeOBBvsOBBMTV(
        const OBBColliderComponent* a, const OBBColliderComponent* b,
        Vector3& outPushForA, Vector3& outPushForB)
    {
        if (!a || !b) return false;

        Vector3 centerA = a->GetCenter();
        Vector3 centerB = b->GetCenter();
        Matrix rotA = a->GetRotationMatrix();
        Matrix rotB = b->GetRotationMatrix();
        Vector3 halfA = a->GetSize() * 0.5f;
        Vector3 halfB = b->GetSize() * 0.5f;

        // 軸抽出
        Vector3 axesA[3], axesB[3];
        // 使用済みユーティリティ (CollisionHelpers::ExtractAxesFromRotation と同等)
        axesA[0] = Vector3(rotA._11, rotA._12, rotA._13);
        axesA[1] = Vector3(rotA._21, rotA._22, rotA._23);
        axesA[2] = Vector3(rotA._31, rotA._32, rotA._33);
        axesB[0] = Vector3(rotB._11, rotB._12, rotB._13);
        axesB[1] = Vector3(rotB._21, rotB._22, rotB._23);
        axesB[2] = Vector3(rotB._31, rotB._32, rotB._33);
        for (int i = 0; i < 3; ++i) { if (axesA[i].LengthSquared() > 1e-6f) axesA[i].Normalize(); }
        for (int i = 0; i < 3; ++i) { if (axesB[i].LengthSquared() > 1e-6f) axesB[i].Normalize(); }

        Vector3 tWorld = centerB - centerA;

        float R[3][3], AbsR[3][3];
        const float EPS = 1e-6f;
        for (int i = 0; i < 3; ++i)
        {
            for (int j = 0; j < 3; ++j)
            {
                R[i][j] = axesA[i].Dot(axesB[j]);
                AbsR[i][j] = std::fabs(R[i][j]) + EPS;
            }
        }

        float tA[3] = { tWorld.Dot(axesA[0]), tWorld.Dot(axesA[1]), tWorld.Dot(axesA[2]) };
        float aHalf[3] = { halfA.x, halfA.y, halfA.z };
        float bHalf[3] = { halfB.x, halfB.y, halfB.z };

        float minOverlap = FLT_MAX;
        Vector3 minAxis = Vector3::Zero;
        bool anyOverlap = false;

        auto consider = [&](const Vector3& axis, float overlap, const Vector3& dir)
            {
                if (overlap <= 0.0f)
                {
                    anyOverlap = false;
                    return false;
                }
                anyOverlap = true;
                if (overlap < minOverlap)
                {
                    minOverlap = overlap;
                    minAxis = dir;
                }
                return true;
            };

        // A の軸
        for (int i = 0; i < 3; ++i)
        {
            float ra = aHalf[i];
            float rb = bHalf[0] * AbsR[i][0] + bHalf[1] * AbsR[i][1] + bHalf[2] * AbsR[i][2];
            float overlap = ra + rb - std::fabs(tA[i]);
            Vector3 dir = axesA[i] * (tA[i] >= 0.0f ? 1.0f : -1.0f);
            if (!consider(axesA[i], overlap, dir)) return false;
        }

        // B の軸
        for (int i = 0; i < 3; ++i)
        {
            float ra = aHalf[0] * AbsR[0][i] + aHalf[1] * AbsR[1][i] + aHalf[2] * AbsR[2][i];
            float proj = std::fabs(tA[0] * R[0][i] + tA[1] * R[1][i] + tA[2] * R[2][i]);
            float overlap = ra + bHalf[i] - proj;
            float projVal = (tA[0] * R[0][i] + tA[1] * R[1][i] + tA[2] * R[2][i]);
            Vector3 dir = axesB[i] * (projVal >= 0.0f ? 1.0f : -1.0f);
            if (!consider(axesB[i], overlap, dir)) return false;
        }

        // 交差軸 Ai x Bj
        for (int i = 0; i < 3; ++i)
        {
            for (int j = 0; j < 3; ++j)
            {
                Vector3 axis = axesA[i].Cross(axesB[j]);
                float axisLenSq = axis.LengthSquared();
                if (axisLenSq < 1e-8f) continue;
                axis.Normalize();

                int i1 = (i + 1) % 3, i2 = (i + 2) % 3;
                int j1 = (j + 1) % 3, j2 = (j + 2) % 3;

                float ra = aHalf[i1] * AbsR[i2][j] + aHalf[i2] * AbsR[i1][j];
                float rb = bHalf[j1] * AbsR[i][j2] + bHalf[j2] * AbsR[i][j1];

                float tProj = std::fabs(tA[i2] * R[i1][j] - tA[i1] * R[i2][j]);
                float overlap = ra + rb - tProj;

                float s = tWorld.Dot(axis);
                Vector3 dir = axis * (s >= 0.0f ? 1.0f : -1.0f);
                if (!consider(axis, overlap, dir)) return false;
            }
        }

        if (!anyOverlap) return false;

        Vector3 mtv = minAxis;
        if (mtv.LengthSquared() > EPS) mtv.Normalize(); else mtv = Vector3::Zero;

        Vector3 push = mtv * minOverlap;
        outPushForA = push;
        outPushForB = -push;
        return true;
    }

    // ResolvePenetration 実装
    bool ResolvePenetration(ColliderComponent* a, ColliderComponent* b)
    {
        if (!a || !b) return false;

        // 既に無効化されているコライダーは無視
        if (!a->IsEnabled() || !b->IsEnabled()) return false;

        Vector3 pushA(0, 0, 0), pushB(0, 0, 0);
        bool collided = false;

        // AABB vs AABB
        if (a->GetColliderType() == ColliderType::AABB && b->GetColliderType() == ColliderType::AABB)
        {
            if (Collision::ComputeAABBMTV(
                static_cast<const AABBColliderComponent*>(a),
                static_cast<const AABBColliderComponent*>(b),
                pushA, pushB))
            {
                collided = true;
            }
        }
        // AABB vs OBB (順不同)
        else if (a->GetColliderType() == ColliderType::AABB && b->GetColliderType() == ColliderType::OBB)
        {
            if (Collision::ComputeAABBvsOBBMTV(
                static_cast<const AABBColliderComponent*>(a),
                static_cast<const OBBColliderComponent*>(b),
                pushA, pushB))
            {
                collided = true;
            }
        }
        else if (a->GetColliderType() == ColliderType::OBB && b->GetColliderType() == ColliderType::AABB)
        {
            // ComputeAABBvsOBBMTV expects (AABB, OBB), but we have (OBB, AABB).
            // swap and then invert pushes.
            Vector3 tmpA, tmpB;
            if (Collision::ComputeAABBvsOBBMTV(
                static_cast<const AABBColliderComponent*>(b),
                static_cast<const OBBColliderComponent*>(a),
                tmpA, tmpB))
            {
                // tmpA is push for b (AABB), tmpB for a (OBB)
                pushA = tmpB;
                pushB = tmpA;
                collided = true;
            }
        }
        // OBB vs OBB
        else // both OBB
        {
            if (ComputeOBBvsOBBMTV(
                static_cast<const OBBColliderComponent*>(a),
                static_cast<const OBBColliderComponent*>(b),
                pushA, pushB))
            {
                collided = true;
            }
        }

        if (!collided) return false;

        // 押し出しの適用：
        GameObject* objA = a->GetOwner();
        GameObject* objB = b->GetOwner();
        if (!objA || !objB)
        {
            return false;
        }

        // 動的かどうかの判定（MoveComponent を持っていれば動的と見なす）
        auto mvA = objA->GetComponent<MoveComponent>();
        auto mvB = objB->GetComponent<MoveComponent>();

        // 配分ルール:
        // - A が動的で B が静的なら A に全押し出しを与える（pushA） 
        // - 両方動的なら半分ずつ（pushA/2 と pushB/2）
        // - 両方静的なら A を全押し出し（後で必要なら改変）
        if (mvA && !mvB)
        {
            // A を移動（位置を直接修正してめり込みを解消）
            objA->SetPosition(objA->GetPosition() + pushA);
        }
        else if (!mvA && mvB)
        {
            objB->SetPosition(objB->GetPosition() + pushB);
        }
        else if (mvA && mvB)
        {
            // 半分ずつ移動（より自然に見える）
            objA->SetPosition(objA->GetPosition() + pushA * 0.5f);
            objB->SetPosition(objB->GetPosition() + pushB * 0.5f);
        }
        else
        {
            // 両方静的: 最低限 A を押す（シーンが壊れる可能性があるのでログ注意）
            objA->SetPosition(objA->GetPosition() + pushA);
        }

        return true;
    }

    bool TryApplyImpulseToObject(GameObject* target, const Vector3& impulse)
    {
        if (!target) return false;
        auto mv = target->GetComponent<MoveComponent>();
        if (mv)
        {
            mv->AddImpulse(impulse);
            return true;
        }
        return false;
    }

    void ApplyKnockback(GameObject* target, const Vector3& direction, float magnitude)
    {
        if (!target) return;
        Vector3 dir = direction;
        if (dir.LengthSquared() < 1e-6f) return;
        dir.Normalize();
        Vector3 impulse = dir * magnitude;
        if (!TryApplyImpulseToObject(target, impulse))
        {
            // MoveComponent を持たない場合は直接位置を少し押し出す（フォールバック）
            target->SetPosition(target->GetPosition() + impulse * 0.02f); // 小さく移動させる
        }
    }
}
