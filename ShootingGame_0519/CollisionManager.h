#pragma once
#include <vector>
#include <memory>
#include "ColliderComponent.h"
#include "DebugRenderer.h"

class DebugRenderer;

//当たっているペを保存する際に使用する構造体
struct CollisionInfoLite
{
    ColliderComponent* a = nullptr;
    ColliderComponent* b = nullptr;
};

class CollisionManager
{
public:

    //当たり判定したいコライダーを今フレームのリストに追加
    static void RegisterCollider(ColliderComponent* collider);

    //当たり判定を登録していた物をリストから削除する
    static void UnregisterCollider(ColliderComponent* collider);

    //前フレームまでに登録されていた
    //コライダーのリスト(m_Colliders)を空にする
    static void Clear();

    //m_Colliders に登録されたコライダーの全組み合わせを判定する関数
    //判定が成功したらOnCollisionを呼び出す
    static void CheckCollisions();

    static void DebugDrawAllColliders(DebugRenderer& dr);

private:

    static void KillInwardVelocity(GameObject* obj,
                            const DirectX::SimpleMath::Vector3& normal);

    //当たり判定を行いたいオブジェクトのリスト
    static std::vector<ColliderComponent*> m_Colliders;
    static bool m_hitThisFrame;
};

