#pragma once
#include <SimpleMath.h>

class AABBColliderComponent;
class OBBColliderComponent;

namespace Collision
{
    bool ComputeAABBvsOBBMTV(
        const DirectX::SimpleMath::Vector3* aabbMinPtr, 
        const DirectX::SimpleMath::Vector3* aabbMaxPtr,
        const void* unused,
        DirectX::SimpleMath::Vector3& outPushForA,
        DirectX::SimpleMath::Vector3& outPushForB);

    bool ComputeAABBvsOBBMTV(
        const class AABBColliderComponent* aabb,
        const class OBBColliderComponent* obb,
        DirectX::SimpleMath::Vector3& outPushForA,
        DirectX::SimpleMath::Vector3& outPushForB);


    bool ComputeAABBMTV(
        const AABBColliderComponent* a,
        const AABBColliderComponent* b,
        DirectX::SimpleMath::Vector3& outPushA, DirectX::SimpleMath::Vector3& outPushB);
}
