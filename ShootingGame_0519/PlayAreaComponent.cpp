#include "PlayAreaComponent.h"
#include "GameScene.h"
#include "RaycastHit.h" 
#include <algorithm>

using namespace DirectX::SimpleMath;

PlayAreaComponent::PlayAreaComponent()
{
}

float PlayAreaComponent::GetGroundHeightAt(const Vector3& p) const
{
    // 簡易版: 平坦地を返す。
    // 将来的には m_scene を使って地形（heightmap）や地面コライダにレイキャストする実装にできます。
    return m_groundY;
}

Vector3 PlayAreaComponent::ResolvePosition(const Vector3& prevPos,
    const Vector3& desiredPos,
    GameObject* owner) const
{
    Vector3 out = desiredPos;

    //AABB 内に閉じる（X/Z と Y 両方）
    if (out.x < m_boundsMin.x) { out.x = m_boundsMin.x; }
    if (out.y < m_boundsMin.y) { out.y = m_boundsMin.y; }
    if (out.z < m_boundsMin.z) { out.z = m_boundsMin.z; }

    if (out.x > m_boundsMax.x) { out.x = m_boundsMax.x; }
    if (out.y > m_boundsMax.y) { out.y = m_boundsMax.y; }
    if (out.z > m_boundsMax.z) { out.z = m_boundsMax.z; }

    // 2) 地面にぶつける（Y を地面より下にしない）
    float groundH = GetGroundHeightAt(out);
    if (out.y < groundH)
    {
        out.y = groundH;
    }

    // 3) 簡易衝突チェック：小範囲で地面の法線や障害物があればここで補正可能
    //    例えば、重なりが酷ければ prevPos に戻す、とか別途レイ投げて傾斜に沿わせる等。

    return out;
}

bool PlayAreaComponent::RaycastObstacle(const Vector3& start,
    const Vector3& dir,
    float length,
    Vector3& outNormal,
    float& outDist,
    GameObject* ignore) const
{
    if (!m_scene) { return false; }

    RaycastHit hit;
    // predicate: decide which objects are considered obstacles (e.g. has collider and not the ignored object)
    auto pred = [ignore](GameObject* obj) -> bool
        {
            if (!obj) { return false; }
            if (obj == ignore) { return false; }
            // ここでオブジェクトの種別やコンポーネントによる絞り込みを行う:
            // 例: AABBColliderComponent* aabb = obj->GetComponent<AABBColliderComponent>();
            //      return aabb != nullptr;
            // 現段階では Enemy 等以外のすべてを候補にする (必要なら絞る)
            return true;
        };

    bool rc = m_scene->Raycast(start, dir, length, hit, pred, ignore);
    if (!rc)
    {
        return false;
    }

    outDist = hit.distance;
    outNormal = hit.normal;
    return true;
}
