#include <iostream>
#include <algorithm>    
#include <cmath>
#include <cfloat>       
#include <DirectXMath.h>
#include <SimpleMath.h> 
#include "Player.h"
#include "CollisionManager.h"
#include "Collision.h"
#include "CollisionResolver.h"
#include "DebugGlobals.h"
#include "renderer.h"
#include "GameObject.h"
#include "MoveComponent.h"
#include "PushOutComponent.h"
#include "IMovable.h"

using namespace DirectX;
using namespace DirectX::SimpleMath;

std::vector<ColliderComponent*> CollisionManager::m_Colliders;
bool CollisionManager::m_hitThisFrame = false;

void CollisionManager::RegisterCollider(ColliderComponent* collider)
{
    if (!collider){ return; }

    if (!collider->GetOwner())
    {
        //ログ表示
        std::cout << "コライダーの所持者が存在しません" << std::endl;
        return;
    }

    //重複防止
    if (std::find(m_Colliders.begin(), m_Colliders.end(), collider) == m_Colliders.end())
    {
        m_Colliders.push_back(collider);
    }
}

void CollisionManager::UnregisterCollider(ColliderComponent* collider)
{
    if (!collider)
    {
        return;
    }

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
    //全コライダーを未ヒット状態にする
    for (auto col : m_Colliders) 
    {
        col->SetHitThisFrame(false);
    }

    //判定する物の収集を行う
    std::vector<CollisionInfoLite>  hitPairs;

    //サイズ取得
    size_t count = m_Colliders.size();
       
    //サイズ分回す
    for (size_t i = 0; i < count; ++i)
    {
        //
        ColliderComponent* colA = m_Colliders[i];
        //コライダーが付いていなければ
        if(!colA){ continue; }

        GameObject* ownerA = colA->GetOwner();
        //所有者がいなければ
        if(!ownerA){ continue; }

        //今の当たり判定の一個先から回す
        for (size_t j = i + 1; j < count; ++j)
        {
            ColliderComponent* colB = m_Colliders[j];
            //コライダーが付いていなければ
            if (!colB){ continue; }

            GameObject* ownerB = colB->GetOwner();
            //所有者がいなければ
            if (!ownerB){ continue; }
            
            bool hit = false;

            //コライダーの種類(AABB or OBB)を取得
            auto typeA = colA->GetColliderType();
            auto typeB = colB->GetColliderType();

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

                //各データを取得する
                Vector3 centerA = a->GetCenter();
                Vector3 centerB = b->GetCenter();
                Matrix  rotA = a->GetRotationMatrix();
                Matrix  rotB = b->GetRotationMatrix();
                Vector3 halfA = a->GetSize() * 0.5f;
                Vector3 halfB = b->GetSize() * 0.5f;

                //
                Vector3 axesA[3] = { rotA.Right(), rotA.Up(), rotA.Forward() };
                Vector3 axesB[3] = { rotB.Right(), rotB.Up(), rotB.Forward() };

                hit = Collision::IsOBBHit(centerA, axesA, halfA, centerB, axesB, halfB);
            }

            //-----------------------------------------
            // 衝突判定 ： AABB vs OBB
            //-----------------------------------------
            else
            {
                AABBColliderComponent* aabb = nullptr;
                OBBColliderComponent* obb = nullptr;
                if (typeA == ColliderType::AABB)
                {
                    aabb = static_cast<AABBColliderComponent*>(colA);
                    obb  = static_cast<OBBColliderComponent*>(colB); 
                }
                else
                {
                    aabb = static_cast<AABBColliderComponent*>(colB); 
                    obb  = static_cast<OBBColliderComponent*>(colA);
                }

                // AABB と OBB 双方の Get* は null-safe 実装を期待
                hit = Collision::IsAABBvsOBBHit(
                    aabb->GetMin(), aabb->GetMax(),
                    obb->GetCenter(), obb->GetRotationMatrix(),
                    obb->GetSize() * 0.5f);
            }

            //-----------------------------------------
            // 結果を送る(ログも出す)
            //-----------------------------------------
            if (hit)
            {
                //コリジョンイベント通知
                //判定フェーズでは通知しないで入れておく。
                CollisionInfoLite info;
                info.a = colA;
                info.b = colB;
                hitPairs.push_back(info);
            }
            else
            {
                //当たっていない場合のログ（デバッグ時のみ有効にするといい）
                //std::cout << "当たっていません "<< std::endl;
            }
        
        }
    }
    
    for (const auto& p : hitPairs)
    {
        //
        if (!p.a || !p.b) { continue; }

        //
        ColliderComponent* colA = p.a;
        ColliderComponent* colB = p.b;

        //
        GameObject* ownerA = colA->GetOwner();
        GameObject* ownerB = colB->GetOwner();

        if (!ownerA || !ownerB) { continue; }

        //押し出し量を保存するための変数を作っておく
        Vector3 pushA = Vector3::Zero;
        Vector3 pushB = Vector3::Zero;

        //当たったどうかのbool型
        bool resolved = false;

        //コライダーの種類を取得する
        ColliderType typeA = colA->GetColliderType();
        ColliderType typeB = colB->GetColliderType();

        //-----------------MTV(押し出し量)計算----------------------
        //AABB同士の当たり判定なら
        if (typeA == ColliderType::AABB && typeB == ColliderType::AABB)
        {
            resolved = Collision::ComputeAABBMTV(static_cast<AABBColliderComponent*>(colA),
                                                 static_cast<AABBColliderComponent*>(colB),
                                                 pushA, 
                                                 pushB);
        }
        //AABBとOBBの当たり判定なら
        if (typeA == ColliderType::AABB && typeB == ColliderType::OBB)
        {
            resolved = Collision::ComputeAABBvsOBBMTV_Simple(static_cast<AABBColliderComponent*>(colA),
                                                             static_cast<OBBColliderComponent*>(colB),
                                                             pushA, pushB);
        }
        else if (typeA == ColliderType::OBB && typeB == ColliderType::AABB)
        {
            resolved = Collision::ComputeAABBvsOBBMTV_Simple(static_cast<AABBColliderComponent*>(colB),
                                                             static_cast<OBBColliderComponent*>(colA),
                                                             pushB, pushA);
        }
        //OBB同士の当たり判定なら
        else if (typeA == ColliderType::OBB && typeB == ColliderType::OBB)
        {
            resolved = Collision::ComputeOBBMTV(static_cast<OBBColliderComponent*>(colA),
                                                static_cast<OBBColliderComponent*>(colB),
                                                pushA,
                                                pushB);
        }
        else
        {
            //それ以外の組み合わせはMTVを計算しない
            resolved = false;
        }

        //押し出しがいらないなら
        if (!resolved){ continue; }
        
        if (resolved)
        {
            auto pushAComp = ownerA->GetComponent<PushOutComponent>();
            auto pushBComp = ownerB->GetComponent<PushOutComponent>();

            float massA;
            if (colA->IsStatic())
            {
                massA = FLT_MAX;
            }
            else
            {
                massA = pushAComp->GetMass();
            }
            
            float massB;
            if (colB->IsStatic())
            {
                massB = FLT_MAX;
            }
            else
            {
                massB = pushBComp->GetMass();
            }

            float invA;
            if (massA >= FLT_MAX)
            {
                invA = 0.0f;
            }
            else
            {
				invA = 1.0f / massA;
            }

            float invB;
            if (massB >= FLT_MAX)
            {
                invB = 0.0f;
            }
            else
            {
                invB = 1.0f / massB;
            }

            float sumInv = invA + invB;
            if (sumInv <= 1e-6f) { continue; }

            Vector3 mtv = pushA; 

            Vector3 finalPushA = mtv * (invA / sumInv);
            Vector3 finalPushB = -mtv * (invB / sumInv);

            if (pushAComp)
            {
                pushAComp->AddPush(finalPushA);
            }
            else
            {
                std::cout << "PushOutComponent missing on A\n";
            }

            if (pushBComp)
            {
                pushBComp->AddPush(finalPushB);
            }
            else
            {
                std::cout << "PushOutComponent missing on B\n";
            }


            Vector3 normal = finalPushA;
            if (normal.LengthSquared() > 1e-6f)
            {
                normal.Normalize();
            }
            else
            {
                normal = Vector3::Up;
            }

            KillInwardVelocity(ownerA, normal);
            KillInwardVelocity(ownerB, -normal);

            colA->SetHitThisFrame(true);
            colB->SetHitThisFrame(true);

            ownerA->OnCollision(ownerB);
            ownerB->OnCollision(ownerA);
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
            sprintf_s(buf, "DB G: AABB center=(%f,%f,%f) fullSize=(%f,%f,%f)\n",
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


void CollisionManager::KillInwardVelocity(GameObject* obj,
                                          const DirectX::SimpleMath::Vector3& normal)
{
    if (!obj){ return; }

    // IMovable を持つか確認
    auto movable = obj->GetComponent<IMovable>();
    if (!movable){ return; }

    Vector3 velocity = movable->GetVelocity();
    float dot = velocity.Dot(normal);

    // 壁の内側へ向かう成分だけ消す
    if (dot < 0.0f)
    {
        Vector3 corrected = velocity - normal * dot;
        movable->SetVelocity(corrected);
    }
}

