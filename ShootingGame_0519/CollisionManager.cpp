#include <iostream>
#include <DirectXMath.h>
#include "CollisionManager.h"
#include "Collision.h" // IsAABBHit, IsOBBHit, IsAABBvsOBB
#include "DebugGlobals.h"
#include "renderer.h"
#include "GameObject.h"

using namespace DirectX;

std::vector<ColliderComponent*> CollisionManager::m_Colliders;

bool CollisionManager::m_hitThisFrame = false;

void CollisionManager::RegisterCollider(ColliderComponent* collider)
{
    if (!collider) return;
    //重複防止
    if (std::find(m_Colliders.begin(), m_Colliders.end(), collider) == m_Colliders.end())
    {
        m_Colliders.push_back(collider);
    }
}

void CollisionManager::UnregisterCollider(ColliderComponent* collider)
{
    if (!collider) return;
    auto it = std::find(m_Colliders.begin(), m_Colliders.end(), collider);
    if (it != m_Colliders.end())
    {
        m_Colliders.erase(it);
    }
}

void CollisionManager::Clear()
{
    m_Colliders.clear();
}

void CollisionManager::CheckCollisions()
{
    //全コライダーを未ヒット状態に
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

                // 判定関数に渡す
                hit = Collision::IsOBBHit(a->GetCenter(), axesA, a->GetSize() * 0.5f, b->GetCenter(), axesB, b->GetSize() * 0.5f);
            }

            //-----------------------------------------
            // 衝突判定 ： AABB vs OBB
            //-----------------------------------------
            else
            {
                decltype(colA) aabb;
                decltype(colA) obb;

                if (typeA == ColliderType::AABB)
                {
                    aabb = colA;
                }
                else
                {
                    aabb = colB;
                }

                if (typeA == ColliderType::OBB)
                {
                    obb = colA;
                }
                else
                {
                    obb = colB;
                }

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
                //当たっている場合のログ
                //std::cout << "当たっています" << std::endl;

                // コリジョンイベント通知
                colA->SetHitThisFrame(true);
                colB->SetHitThisFrame(true);
                colA->GetOwner()->OnCollision(colB->GetOwner());
                colB->GetOwner()->OnCollision(colA->GetOwner());
            }
            else
            {
                //当たっていない場合のログ（デバッグ時のみ有効にするといい）
                //std::cout << "当たっていません "<< std::endl;
            }
        }
    }

}

void CollisionManager::DebugDrawAllColliders(DebugRenderer& dr)
{
    if (m_Colliders.empty()) 
    {
        std::cout << "コライダーに何も登録されていません " << std::endl;
        return;
    }

    // 深度オフにして両面描画で見やすく
    //m_DeviceContextBackup... // （もし保存できるなら保存、無ければ Renderer 関数で切り替え）
    Renderer::SetDepthEnable(false);
    Renderer::DisableCulling(false);

    for (auto* col : m_Colliders)
    {
        if (!col) continue;
        bool hit = col->IsHitThisFrame();
        Vector4 color = hit ? Vector4(1, 0, 0, 1) : Vector4(0, 1, 0, 0.7f);

        if (col->GetColliderType() == ColliderType::AABB)
        {
            auto* a = static_cast<AABBColliderComponent*>(col);
            Vector3 mn = a->GetMin();
            Vector3 mx = a->GetMax();
            Vector3 center = (mn + mx) * 0.5f;
            Vector3 fullSize = (mx - mn);
            Vector3 halfSize = fullSize * 0.5f;

            char buf[256];
            sprintf_s(buf, "DBG: AABB center=(%f,%f,%f) fullSize=(%f,%f,%f)\n",
                center.x, center.y, center.z, fullSize.x, fullSize.y, fullSize.z);
            OutputDebugStringA(buf);

            fullSize = halfSize * 2.0f;
            dr.AddBox(center, fullSize, Matrix::Identity, color);
        }
        else // OBB
        {
            auto* o = static_cast<OBBColliderComponent*>(col);
            Vector3 center = o->GetCenter();
            Vector3 fullSize = o->GetSize();
            Vector3 halfSize = fullSize * 0.5f;
            Matrix rot = o->GetRotationMatrix();

            char buf[256];
            sprintf_s(buf, "DBG: OBB center=(%f,%f,%f) fullSize=(%f,%f,%f)\n",
                center.x, center.y, center.z, fullSize.x, fullSize.y, fullSize.z);
            OutputDebugStringA(buf);

            dr.AddBox(center, halfSize, rot, color);
        }
    }

    // 元に戻す
    Renderer::SetDepthEnable(true);
    Renderer::DisableCulling(true);
}


//void CollisionManager::DebugDrawAllColliders(DebugRenderer& dr)
//{
//    // 見えやすく一時的にデプスオフ、カリングオフ
//    Renderer::SetDepthEnable(false);
//    Renderer::DisableCulling(false);
//
//    for (auto* col : m_Colliders)
//    {
//        bool hit = col->IsHitThisFrame();
//        Vector4 color = hit ? Vector4(1, 0, 0, 1) : Vector4(0, 1, 0, 0.7f);
//
//        if (col->GetColliderType() == ColliderType::AABB)
//        {
//            auto* a = static_cast<AABBColliderComponent*>(col);
//            Vector3 mn = a->GetMin();
//            Vector3 mx = a->GetMax();
//            Vector3 center = (mn + mx) * 0.5f;
//            Vector3 fullSize = (mx - mn);
//            Vector3 halfSize = fullSize * 0.5f;
//
//            char buf[256];
//            sprintf_s(buf, "DBG: AABB center=(%f,%f,%f) fullSize=(%f,%f,%f)\n",
//                center.x, center.y, center.z, fullSize.x, fullSize.y, fullSize.z);
//            OutputDebugStringA(buf);
//
//            dr.AddBox(center, halfSize, Matrix::Identity, color);
//        }
//        else // OBB
//        {
//            auto* o = static_cast<OBBColliderComponent*>(col);
//            Vector3 center = o->GetCenter();
//            Vector3 fullSize = o->GetSize();
//            Vector3 halfSize = fullSize * 0.5f;
//            Matrix rot = o->GetRotationMatrix();
//
//            char buf[256];
//            sprintf_s(buf, "DBG: OBB center=(%f,%f,%f) fullSize=(%f,%f,%f)\n",
//                center.x, center.y, center.z, fullSize.x, fullSize.y, fullSize.z);
//            OutputDebugStringA(buf);
//
//            dr.AddBox(center, halfSize, rot, color);
//        }
//    }
//
//    // restore
//    Renderer::SetDepthEnable(true);
//    Renderer::DisableCulling(true);
//}



