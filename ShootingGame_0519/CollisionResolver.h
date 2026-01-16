#pragma once
#include <SimpleMath.h>

class AABBColliderComponent;
class OBBColliderComponent;
class SphereColliderComponent;

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
        DirectX::SimpleMath::Vector3& outPushA,
        DirectX::SimpleMath::Vector3& outPushB);

    bool ComputeAABBMTV(
        const  DirectX::SimpleMath::Vector3& aMin,
        const  DirectX::SimpleMath::Vector3& aMax,
        const  DirectX::SimpleMath::Vector3& bMin,
        const  DirectX::SimpleMath::Vector3& bMax,
        DirectX::SimpleMath::Vector3& outPushA,
        DirectX::SimpleMath::Vector3& outPushB);

    bool ComputeAABBvsOBBMTV_Simple(const AABBColliderComponent* aabb,
        const OBBColliderComponent* obb,
        DirectX::SimpleMath::Vector3& outPushForA,
        DirectX::SimpleMath::Vector3& outPushForB);

    bool ComputeOBBMTV(
        const OBBColliderComponent* a,
        const OBBColliderComponent* b,
        DirectX::SimpleMath::Vector3& outPushForA,
        DirectX::SimpleMath::Vector3& outPushForB);

    bool ComputeSphereVsOBBMTV(
        const SphereColliderComponent* sphere,
        const OBBColliderComponent* obb,
        DirectX::SimpleMath::Vector3& outPushForSphere,
        DirectX::SimpleMath::Vector3& outPushForObb);
}
