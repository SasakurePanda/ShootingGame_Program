#include <iostream>
#include <DirectXMath.h>
#include "CollisionManager.h"
#include "Collision.h" // IsAABBHit, IsOBBHit, IsAABBvsOBB
#include "DebugGlobals.h"

using namespace DirectX;

std::vector<ColliderComponent*> CollisionManager::m_Colliders;

bool CollisionManager::m_hitThisFrame = false;

void CollisionManager::RegisterCollider(ColliderComponent* collider)
{
    m_Colliders.push_back(collider);
}

void CollisionManager::Clear()
{
    m_Colliders.clear();
}

void CollisionManager::CheckCollisions()
{
    // まず全コライダーを未ヒット状態に
    for (auto col : m_Colliders)
        col->SetHitThisFrame(false);

    size_t count = m_Colliders.size();
    for (size_t i = 0; i < count; ++i)
    {
        for (size_t j = i + 1; j < count; ++j)
        {
            ColliderComponent* colA = m_Colliders[i];
            ColliderComponent* colB = m_Colliders[j];

            //コライダーの種類(AABB or OBB)を取得
            auto typeA = colA->GetColliderType();
            auto typeB = colB->GetColliderType();

            bool hit = false;

            //-----------------------------------------
            // 衝突判定 ： AABB vs AABB
            //-----------------------------------------
            if (typeA == ColliderType::AABB && typeB == ColliderType::AABB)
            {
                auto a = static_cast<AABBColliderComponent*>(colA);
                auto b = static_cast<AABBColliderComponent*>(colB);
                hit = Collision::IsAABBHit(a->GetMin(), a->GetMax(), b->GetMin(), b->GetMax());
            }
            //-----------------------------------------
            // 衝突判定 ： OBB vs OBB
            //-----------------------------------------
            else if (typeA == ColliderType::OBB && typeB == ColliderType::OBB)
            {
                auto a = static_cast<OBBColliderComponent*>(colA);
                auto b = static_cast<OBBColliderComponent*>(colB);

                // 回転（度数）を取得
                Vector3 rotA_deg = a->GetOwner()->GetRotation();
                Vector3 rotB_deg = b->GetOwner()->GetRotation();

                // ラジアンに変換
                Vector3 rotA_rad = Vector3(
                    XMConvertToRadians(rotA_deg.x),
                    XMConvertToRadians(rotA_deg.y),
                    XMConvertToRadians(rotA_deg.z)
                );
                Vector3 rotB_rad = Vector3(
                    XMConvertToRadians(rotB_deg.x),
                    XMConvertToRadians(rotB_deg.y),
                    XMConvertToRadians(rotB_deg.z)
                );

                // 回転行列生成（YawPitchRoll順）
                Matrix rotA = Matrix::CreateFromYawPitchRoll(rotA_rad.y, rotA_rad.x, rotA_rad.z);
                Matrix rotB = Matrix::CreateFromYawPitchRoll(rotB_rad.y, rotB_rad.x, rotB_rad.z);

                // 軸ベクトル計算
                Vector3 axesA[3] = { rotA.Right(), rotA.Up(), rotA.Forward() };
                Vector3 axesB[3] = { rotB.Right(), rotB.Up(), rotB.Forward() };

                // 軸ベクトルの長さチェック（デバッグ用）
                //for (int i = 0; i < 3; i++) {
                //    std::cout << "AxisA[" << i << "] length: " << axesA[i].Length() << std::endl;
                //    std::cout << "AxisB[" << i << "] length: " << axesB[i].Length() << std::endl;
                //}

                // 判定関数に渡す
                hit = Collision::IsOBBHit(a->GetCenter(), axesA, a->GetSize() * 0.5f,
                    b->GetCenter(), axesB, b->GetSize() * 0.5f);
            }

            //-----------------------------------------
            // 衝突判定 ： AABB vs OBB
            //-----------------------------------------
            else
            {
                auto aabb = (typeA == ColliderType::AABB) ? colA : colB;
                auto obb = (typeA == ColliderType::OBB) ? colA : colB;

                auto a = static_cast<AABBColliderComponent*>(aabb);
                auto b = static_cast<OBBColliderComponent*>(obb);

                hit = Collision::IsAABBvsOBBHit(a->GetMin(), a->GetMax(),
                    b->GetCenter(), b->GetRotationMatrix(), b->GetSize() * 0.5f);
            }

            //-----------------------------------------
            // 結果を送る(ログも出す)
            //-----------------------------------------
            if (hit)
            {
                // ✅ 当たっている場合のログ
                //std::cout << "当たっています" << std::endl;

                // コリジョンイベント通知
                colA->SetHitThisFrame(true);
                colB->SetHitThisFrame(true);
                colA->GetOwner()->OnCollision(colB->GetOwner());
                colB->GetOwner()->OnCollision(colA->GetOwner());
            }
            else
            {
                // ✅ 当たっていない場合のログ（デバッグ時のみ有効にするといい）
                //std::cout << "当たっていません "<< std::endl;
            }
        }
    }

}

void CollisionManager::DebugDrawAllColliders(DebugRenderer& dr) 
{
    for (auto* col : m_Colliders)
    {
        bool hit = col->IsHitThisFrame();
        Vector4 color = hit ? Vector4(1, 0, 0, 1) : Vector4(0, 1, 0, 0.7f);

        if (col->GetColliderType() == ColliderType::AABB)
        {
            auto* a = static_cast<AABBColliderComponent*>(col);
            Vector3 mn = a->GetMin();
            Vector3 mx = a->GetMax();
            Vector3 center = (mn + mx) * 0.5f;
            Vector3 size = (mx - mn);
            dr.AddBox(center, size, Matrix::Identity, color);
        }
        else // OBB
        {
            auto* o = static_cast<OBBColliderComponent*>(col);
            dr.AddBox(o->GetCenter(), o->GetSize(), o->GetRotationMatrix(), color);
        }
    }
}


