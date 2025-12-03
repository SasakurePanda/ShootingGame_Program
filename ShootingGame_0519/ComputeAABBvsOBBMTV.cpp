#include <SimpleMath.h>
#include "AABBColliderComponent.h"
#include "OBBColliderComponent.h"

using namespace DirectX::SimpleMath;

namespace Collision
{
    //// helper: project half extents of OBB onto an arbitrary axis (unit axis)
    //static float ProjectHalfSizeOnAxis_OBB(const Vector3 halfB, const Vector3 axesB[3], const Vector3& axis)
    //{
    //    // sum(|axis·Bi| * halfB_i)
    //    return fabsf(axis.Dot(axesB[0])) * halfB.x
    //        + fabsf(axis.Dot(axesB[1])) * halfB.y
    //        + fabsf(axis.Dot(axesB[2])) * halfB.z;
    //}

    //static float ProjectHalfSizeOnAxis_AABB(const Vector3 halfA, const Vector3& axis)
    //{
    //    // AABB axes are world axes: (1,0,0),(0,1,0),(0,0,1)
    //    return fabsf(axis.x) * halfA.x + fabsf(axis.y) * halfA.y + fabsf(axis.z) * halfA.z;
    //}

    //bool ComputeAABBvsOBBMTV(const AABBColliderComponent* aabb, const OBBColliderComponent* obb,
    //    Vector3& outPushA, Vector3& outPushB)
    //{
    //    if (!aabb || !obb) return false;

    //    Vector3 aMin = aabb->GetMin();
    //    Vector3 aMax = aabb->GetMax();
    //    Vector3 centerA = (aMin + aMax) * 0.5f;
    //    Vector3 halfA = (aMax - aMin) * 0.5f;

    //    Vector3 centerB = obb->GetCenter();
    //    Vector3 halfB = obb->GetSize() * 0.5f;

    //    // axesB from rotation matrix
    //    Matrix rotB = obb->GetRotationMatrix();
    //    Vector3 axesB[3];
    //    axesB[0] = Vector3(rotB._11, rotB._12, rotB._13);
    //    axesB[1] = Vector3(rotB._21, rotB._22, rotB._23);
    //    axesB[2] = Vector3(rotB._31, rotB._32, rotB._33);
    //    axesB[0].Normalize(); axesB[1].Normalize(); axesB[2].Normalize();

    //    // candidate axes: world X,Y,Z and OBB axes (3) and cross products (3x3) -> up to 15
    //    std::vector<Vector3> axes;
    //    axes.reserve(15);
    //    axes.push_back(Vector3::UnitX);
    //    axes.push_back(Vector3::UnitY);
    //    axes.push_back(Vector3::UnitZ);
    //    axes.push_back(axesB[0]);
    //    axes.push_back(axesB[1]);
    //    axes.push_back(axesB[2]);

    //    for (int i = 0; i < 3; ++i)
    //    {
    //        for (int j = 0; j < 3; ++j)
    //        {
    //            Vector3 c = axesB[j].Cross((i == 0 ? Vector3::UnitX : (i == 1 ? Vector3::UnitY : Vector3::UnitZ)));
    //            if (c.LengthSquared() > 1e-6f)
    //            {
    //                c.Normalize();
    //                axes.push_back(c);
    //            }
    //        }
    //    }

    //    // track smallest overlap
    //    float smallestOverlap = FLT_MAX;
    //    Vector3 smallestAxis = Vector3::Zero;
    //    float centerDistSign = 1.0f;

    //    Vector3 d = centerB - centerA;

    //    for (const Vector3& axisRaw : axes)
    //    {
    //        Vector3 axis = axisRaw;
    //        if (axis.LengthSquared() < 1e-6f) continue;
    //        axis.Normalize();

    //        float projA = ProjectHalfSizeOnAxis_AABB(halfA, axis);
    //        float projB = ProjectHalfSizeOnAxis_OBB(halfB, axesB, axis);
    //        float dist = fabsf(d.Dot(axis));

    //        float overlap = projA + projB - dist;
    //        if (overlap <= 0.0f)
    //        {
    //            // separating axis found -> no collision (shouldn't happen because we called only when hit=true)
    //            return false;
    //        }

    //        if (overlap < smallestOverlap)
    //        {
    //            smallestOverlap = overlap;
    //            smallestAxis = axis;
    //            centerDistSign = (d.Dot(axis) >= 0.0f) ? 1.0f : -1.0f;
    //        }
    //    }

    //    if (smallestOverlap == FLT_MAX)
    //    {
    //        return false;
    //    }

    //    // Direction: move A away from B along smallestAxis
    //    // if centerB is on positive side along axis, move A negative (i.e. -axis)
    //    Vector3 push = -smallestAxis * (smallestOverlap * centerDistSign);

    //    outPushA = push;
    //    outPushB = -push;
    //    return true;
    //}
}