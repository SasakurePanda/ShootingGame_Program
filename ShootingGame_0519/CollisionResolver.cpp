#include "CollisionResolver.h"
#include "CollisionHelpers.h"
#include "AABBColliderComponent.h"
#include "OBBColliderComponent.h"
#include "SphereColliderComponent.h"

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <limits>

using namespace DirectX::SimpleMath;

namespace Collision
{
    static const float EPS = 1e-6f;

    static void GetOBBWorldAABB(const OBBColliderComponent* obb,
        DirectX::SimpleMath::Vector3& outMin,
        DirectX::SimpleMath::Vector3& outMax)
    {
        using namespace DirectX::SimpleMath;

        if (!obb)
        {
            outMin = Vector3::Zero;
            outMax = Vector3::Zero;
            return;
        }

        // 中心・回転・半サイズを取得
        Vector3 center = obb->GetCenter();
        Matrix  rot = obb->GetRotationMatrix();
        Vector3 half = obb->GetSize() * 0.5f;

        // OBB のローカル軸（Right, Up, Forward）
        Vector3 axisX = Vector3(rot._11, rot._12, rot._13);
        Vector3 axisY = Vector3(rot._21, rot._22, rot._23);
        Vector3 axisZ = Vector3(rot._31, rot._32, rot._33);

        if (axisX.LengthSquared() > 1e-6f)
        {
            axisX.Normalize();
        }
        if (axisY.LengthSquared() > 1e-6f)
        {
            axisY.Normalize();
        }
        if (axisZ.LengthSquared() > 1e-6f)
        {
            axisZ.Normalize();
        }

        Vector3 corners[8];
        int index = 0;
        for (int sx = -1; sx <= 1; sx += 2)
        {
            for (int sy = -1; sy <= 1; sy += 2)
            {
                for (int sz = -1; sz <= 1; sz += 2)
                {
                    Vector3 offset = axisX * (half.x * (float)sx)
                        + axisY * (half.y * (float)sy)
                        + axisZ * (half.z * (float)sz);
                    corners[index] = center + offset;
                    index++;
                }
            }
        }

        // 最初のコーナーで初期化
        outMin = corners[0];
        outMax = corners[0];

        // 残りから min / max を更新
        for (int i = 1; i < 8; ++i)
        {
            const Vector3& c = corners[i];

            if (c.x < outMin.x)
            {
                outMin.x = c.x;
            }
            if (c.y < outMin.y)
            {
                outMin.y = c.y;
            }
            if (c.z < outMin.z)
            {
                outMin.z = c.z;
            }

            if (c.x > outMax.x)
            {
                outMax.x = c.x;
            }
            if (c.y > outMax.y)
            {
                outMax.y = c.y;
            }
            if (c.z > outMax.z)
            {
                outMax.z = c.z;
            }
        }
    }



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

    bool ComputeAABBMTV(
        const Vector3& aMin,
        const Vector3& aMax,
        const Vector3& bMin,
        const Vector3& bMax,
        Vector3& outPushA,
        Vector3& outPushB)
    {
        using namespace DirectX::SimpleMath;

        // overlaps along each axis
        float overlapX = std::min(aMax.x, bMax.x) - std::max(aMin.x, bMin.x);
        float overlapY = std::min(aMax.y, bMax.y) - std::max(aMin.y, bMin.y);
        float overlapZ = std::min(aMax.z, bMax.z) - std::max(aMin.z, bMin.z);

        if (overlapX <= 0.0f || overlapY <= 0.0f || overlapZ <= 0.0f)
        {
            return false;
        }

        float minOverlap = overlapX;
        int axis = 0;
        if (overlapY < minOverlap)
        {
            minOverlap = overlapY;
            axis = 1;
        }
        if (overlapZ < minOverlap)
        {
            minOverlap = overlapZ;
            axis = 2;
        }

        Vector3 centerA = (aMin + aMax) * 0.5f;
        Vector3 centerB = (bMin + bMax) * 0.5f;
        Vector3 dir = centerA - centerB;

        if (dir.LengthSquared() < 1e-6f)
        {
            dir = Vector3(0, 1, 0);
        }

        Vector3 push = Vector3::Zero;
        if (axis == 0)
        {
            if (dir.x >= 0.0f) push.x = minOverlap;
            else                push.x = -minOverlap;
        }
        else if (axis == 1)
        {
            if (dir.y >= 0.0f) push.y = minOverlap;
            else                push.y = -minOverlap;
        }
        else  // axis == 2
        {
            if (dir.z >= 0.0f) push.z = minOverlap;
            else                push.z = -minOverlap;
        }

        outPushA = push;
        outPushB = -push;

        return true;
    }


    bool ComputeAABBvsOBBMTV(const AABBColliderComponent* aabb,
                             const OBBColliderComponent* obb,
                             Vector3& outPushForA,
                             Vector3& outPushForB)
    {
        if (!aabb || !obb)
        {
            return false;
        }

        //-----------ABB の中心と半幅(ワールド)-----------
        Vector3 aMin = aabb->GetMin();  //箱の小さい部分
        Vector3 aMax = aabb->GetMax();  //箱の大きい部分
        
        Vector3 aCenterV = (aMin + aMax) * 0.5f;  //箱の中心
        Vector3 aHalfV   = (aMax - aMin) * 0.5f;  //箱の半幅

        //-----------OBB の中心と半幅、軸(ワールド)-----------
        Vector3 bCenter = obb->GetCenter();         //箱の中心
        Vector3 bHalfV  = obb->GetSize() * 0.5f;    //箱の半幅
        Matrix  rotB    = obb->GetRotationMatrix(); //箱の回転分

        //AABBの三軸を抽出(AABBは回転がない)
        Vector3 axesA[3] = { Vector3(1,0,0), Vector3(0,1,0), Vector3(0,0,1) };

        //OBBの三軸を抽出
        Vector3 axesB[3];
        //回転行列からOBBの三軸を生成
        ExtractAxesFromRotation(rotB, axesB);

        //AABB の中心 → OBB の中心へのベクトル
        Vector3 tWorld = bCenter - aCenterV;

        //軸同士がどれくらい傾いているかを
        //内積等を利用して実施しています
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

        //中心の距離で
        //Aの軸方向からどれぐらい離れているかを計算
        float tA[3] = { tWorld.Dot(axesA[0]), 
                        tWorld.Dot(axesA[1]), 
                        tWorld.Dot(axesA[2]) };

        //箱半分の位置を分けて保存？
        float aHalf[3] = { aHalfV.x, aHalfV.y, aHalfV.z };
        float bHalf[3] = { bHalfV.x, bHalfV.y, bHalfV.z };

        //最小のオーバーラップを探す
        //(一応あり得る最大値を入れておく)
        float minOverlap = FLT_MAX;
        Vector3 minAxis = Vector3::Zero;

        //見つかったかのboolを作っておく
        bool found = false;

        //各軸の重なり量(overlap)を調べて最も小さいものを記録する関数
        auto testAxis = [&](const Vector3& axis, 
                            float overlap, 
                            const Vector3& axisDirCandidate)
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

        //----------------------Aの軸(AABBの方)をチェック----------------------
        for (int i = 0; i < 3; ++i)
        { 
            //半分の各軸の半径を入れておく
            float ra = aHalf[i];   

            //OBBをAの軸に投影した際の投影幅を計算する
            //各軸のAABBの半径 * 内積の絶対値を計算して足す
            float rb = bHalf[0] * AbsR[i][0] + bHalf[1] * AbsR[i][1] + bHalf[2] * AbsR[i][2];

            //半径と投影幅を足して軸からの距離の絶対値を引く
            float overlap = ra + rb - std::fabs(tA[i]);

            //AABBの軸を入れる
            Vector3 axis = axesA[i];


            float sign;
            //軸が0以上
            if (tA[i] >= 0.0f) 
            {
                sign = 1.0f;
            }
            else
            {
                sign = -1.0f;
            }

            //軸に決まった符号をかける
            Vector3 dir = axis * sign;

            //重なり量計算
            testAxis(axis, overlap, dir);

            //重なりが見つからなかったor重なりが0より小さいなら
            if (!found && overlap <= 0.0f)
            {
                return false;
            }
        }

        //----------------------Bの軸(OBBの方)をチェック----------------------
        for (int i = 0; i < 3; ++i)
        {
            //OBBをAの軸に投影した際の投影幅を計算する
            //各軸のAABBの半径 * 内積の絶対値を計算して足す
            float ra = aHalf[0] * AbsR[0][i] + aHalf[1] * AbsR[1][i] + aHalf[2] * AbsR[2][i];
            
            //半分の各軸の半径を入れておく
            float rb = bHalf[i];

            //中心間距離の軸方向成分計算する
            float tProj = std::fabs(tA[0] * R[0][i] + tA[1] * R[1][i] + tA[2] * R[2][i]);

            //重なり量を計算する
            float overlap = ra + rb - tProj;

            //
            float projVal = (tA[0] * R[0][i] + tA[1] * R[1][i] + tA[2] * R[2][i]);

            float projSign;
            if (projVal >= 0.0f) 
            {
                projSign = 1.0f;
            }
            else 
            {
                projSign = -1.0f;
            }

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
                float sign;
                if (s >= 0.0f)
                {
                    sign = 1.0f;
                }
                else
                {
                    sign = -1.0f;
                }

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


    bool ComputeAABBvsOBBMTV_Simple(const AABBColliderComponent* aabb,
        const OBBColliderComponent* obb,
        DirectX::SimpleMath::Vector3& outPushForA,
        DirectX::SimpleMath::Vector3& outPushForB)
    {
        using namespace DirectX::SimpleMath;

        if (!aabb || !obb)
        {
            return false;
        }

        // AABB 側の min / max
        Vector3 aMin = aabb->GetMin();
        Vector3 aMax = aabb->GetMax();

        // OBB 側を「外接 AABB」に変換
        Vector3 bMin;
        Vector3 bMax;
        GetOBBWorldAABB(obb, bMin, bMax);

        //既に動いている AABB vs AABB MTV を使う
        return ComputeAABBMTV(aMin, aMax,
            bMin, bMax,
            outPushForA,
            outPushForB);
    }

   
    bool ComputeAABBMTV(const AABBColliderComponent* a, const AABBColliderComponent* b,
        Vector3& outPushA, Vector3& outPushB)
    {
        if (!a || !b)
        {
            return false;

        }

        Vector3 aMin = a->GetMin(); //A箱の一番小さい部分
        Vector3 aMax = a->GetMax(); //A箱の一番大きい部分
        Vector3 bMin = b->GetMin(); //B箱の一番小さい部分
        Vector3 bMax = b->GetMax(); //B箱の一番大きい部分

        //重なり量を計算する
        //A----------
        //       ----  ←ここの量
        //      B-----------
        float overlapX = std::min(aMax.x, bMax.x) - std::max(aMin.x, bMin.x);
        float overlapY = std::min(aMax.y, bMax.y) - std::max(aMin.y, bMin.y);
        float overlapZ = std::min(aMax.z, bMax.z) - std::max(aMin.z, bMin.z);

        //重なりがない軸であれば何もしない
        if (overlapX <= 0.0f || overlapY <= 0.0f || overlapZ <= 0.0f)
        {
            return false;
        }

        //
        float minOverlap = overlapX;
        int axis = 0;         //０ならX、１ならY、２ならZ

        //Yの押し出し量の方が今の一番小さい押し出し量よりも
        //小さいなら
        if (overlapY < minOverlap)
        {
            minOverlap = overlapY; 
            axis = 1;
        }

        //Zの押し出し量の方が今の一番小さい押し出し量よりも
        //小さいなら
        if (overlapZ < minOverlap) 
        {
            minOverlap = overlapZ;
            axis = 2; 
        }

        //箱の真ん中を計算する
        Vector3 centerA = (aMin + aMax) * 0.5f;     //Aの箱の真ん中
        Vector3 centerB = (bMin + bMax) * 0.5f;     //Bの箱の真ん中
        Vector3 dir = centerA - centerB;            //ふたつの箱の中心の位置の差

        //
        if (dir.LengthSquared() < 1e-6f)
        {
            dir = Vector3(0, 1, 0); // fallback
        }

        Vector3 push = Vector3::Zero;

        switch (axis)
        {
        case 0://Xに押し出す
            if (dir.x >= 0.0f)
            {
                //A は B の右側 → 右へ押す
                push.x = minOverlap;
            }
            else
            {
                //A は B の左側 → 左へ押す
                push.x = -minOverlap;
            }
            break;

        case 1://Yに押し出す
            if (dir.y >= 0.0f)
            {
                //A は B の上側
                push.y = minOverlap;
            }
            else
            {
                //A は B の下側
                push.y = -minOverlap;
            }
            break;

        case 2://Zに押し出す
            if (dir.z >= 0.0f)
            {
                //A は B の前側（+Z）
                push.z = minOverlap;
            }
            else
            {
                //A は B の後ろ側（-Z）
                push.z = -minOverlap;
            }
            break;
        }

        //各AとBの箱に押し出す量を入れる
        outPushA =  push;   
        outPushB = -push;   
        return true;
    }

    bool ComputeOBBMTV(const OBBColliderComponent* a, 
                       const OBBColliderComponent* b,
                       Vector3& outPushForA, Vector3& outPushForB)
    {
        using namespace DirectX::SimpleMath;

        if (!a || !b)
        {
            return false;
        }

        // 基本データ取得
        Vector3 centerA = a->GetCenter();
        Vector3 centerB = b->GetCenter();
        Matrix rotA = a->GetRotationMatrix();
        Matrix rotB = b->GetRotationMatrix();
        Vector3 halfA = a->GetSize() * 0.5f;
        Vector3 halfB = b->GetSize() * 0.5f;

        // 軸抽出（Right, Up, Forward）
        Vector3 axesA[3] = 
        {
            Vector3(rotA._11, rotA._12, rotA._13),
            Vector3(rotA._21, rotA._22, rotA._23),
            Vector3(rotA._31, rotA._32, rotA._33)
        };

        Vector3 axesB[3] = 
        {
            Vector3(rotB._11, rotB._12, rotB._13),
            Vector3(rotB._21, rotB._22, rotB._23),
            Vector3(rotB._31, rotB._32, rotB._33)
        };

        for (int i = 0; i < 3; ++i)
        {
            if (axesA[i].LengthSquared() > 1e-6f)
            {
                axesA[i].Normalize();
            }
            if (axesB[i].LengthSquared() > 1e-6f)
            {
                axesB[i].Normalize();
            }
        }

        Vector3 tWorld = centerB - centerA;

        const float EPS = 1e-6f;
        float R[3][3];
        float AbsR[3][3];
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

        auto considerAxis = [&](const Vector3& axis, float overlap, const Vector3& dir) -> bool
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

        for (int i = 0; i < 3; ++i)
        {
            float ra = aHalf[i];
            float rb = bHalf[0] * AbsR[i][0] + bHalf[1] * AbsR[i][1] + bHalf[2] * AbsR[i][2];
            float overlap = ra + rb - std::fabs(tA[i]);

            // 符号方向を決める（tA[i] の符号に依存）
            float sign = 1.0f;
            if (tA[i] < 0.0f)
            {
                sign = -1.0f;
            }
            Vector3 dir = axesA[i] * sign;

            if (!considerAxis(axesA[i], overlap, dir))
            {
                return false;
            }
        }

        for (int i = 0; i < 3; ++i)
        {
            float ra = aHalf[0] * AbsR[0][i] + aHalf[1] * AbsR[1][i] + aHalf[2] * AbsR[2][i];
            float rb = bHalf[i];

            float proj = tA[0] * R[0][i] + tA[1] * R[1][i] + tA[2] * R[2][i];
            float tProj = std::fabs(proj);

            float overlap = ra + rb - tProj;

            float projSign = 1.0f;
            if (proj < 0.0f)
            {
                projSign = -1.0f;
            }
            Vector3 dir = axesB[i] * projSign;

            if (!considerAxis(axesB[i], overlap, dir))
            {
                return false;
            }
        }

        // --- 交差軸 Ai x Bj チェック (9個) ---
        for (int i = 0; i < 3; ++i)
        {
            for (int j = 0; j < 3; ++j)
            {
                Vector3 axis = axesA[i].Cross(axesB[j]);
                float axisLenSq = axis.LengthSquared();
                if (axisLenSq < 1e-8f)
                {
                    // 並行に近い軸はスキップ
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
                float sign = 1.0f;
                if (s < 0.0f)
                {
                    sign = -1.0f;
                }
                Vector3 dir = axis * sign;

                if (!considerAxis(axis, overlap, dir))
                {
                    return false;
                }
            }
        }

        if (!anyOverlap)
        {
            return false;
        }

        // minAxis を法線として MTV を作る
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

        // safety clamp: とても大きな push は禁止（デバッグ目的で入れておくと良い）
        const float maxPush = 50.0f;
        if (push.Length() > maxPush)
        {
            push = push * (maxPush / push.Length());
        }

        outPushForA = push;
        outPushForB = -push;

        /*float totalWeight = weightA + weightB;
          Vector3 pushA =  MTV * (weightB / totalWeight);
          Vector3 pushB = -MTV * (weightA / totalWeight);*/

        return true;
    }

    bool ComputeSphereVsOBBMTV(
        const SphereColliderComponent* sphere,
        const OBBColliderComponent* obb,
        Vector3& outPushForSphere,
        Vector3& outPushForObb)
    {
        if (!sphere || !obb)
        {
            return false;
        }

        Vector3 c = sphere->GetCenter();
        float r = sphere->GetRadius();

        Vector3 obbCenter = obb->GetCenter();
        Vector3 obbHalf = obb->GetSize() * 0.5f;

        Vector3 axes[3];
        ExtractAxesFromRotation(obb->GetRotationMatrix(), axes);

        Vector3 p = ClosestPtPointOBB(c, obbCenter, axes, obbHalf);
        Vector3 v = c - p;
        float distSq = v.LengthSquared();

        // 外側から当たってる一般ケース
        if (distSq > 1e-8f)
        {
            float dist = std::sqrt(distSq);
            if (dist >= r)
            {
                return false;
            }

            Vector3 n = v / dist;                 // 押し出し方向
            float penetration = (r - dist);       // めり込み量
            Vector3 push = n * penetration;

            outPushForSphere = push;
            outPushForObb = -push;
            return true;
        }

        // ここに来るのは「球中心がOBB内側」または「完全に面上」など
        // -> 一番近い面方向へ押し出す（安定のため）
        Vector3 d = c - obbCenter;

        float localX = d.Dot(axes[0]);
        float localY = d.Dot(axes[1]);
        float localZ = d.Dot(axes[2]);

        float dx = obbHalf.x - std::fabs(localX);
        float dy = obbHalf.y - std::fabs(localY);
        float dz = obbHalf.z - std::fabs(localZ);

        // 最短で外に出る軸を選ぶ
        float minDist = dx;
        int axis = 0;

        if (dy < minDist)
        {
            minDist = dy;
            axis = 1;
        }
        if (dz < minDist)
        {
            minDist = dz;
            axis = 2;
        }

        Vector3 n = axes[axis];
        float sign = 1.0f;

        if (axis == 0)
        {
            if (localX < 0.0f) { sign = -1.0f; }
        }
        else if (axis == 1)
        {
            if (localY < 0.0f) { sign = -1.0f; }
        }
        else
        {
            if (localZ < 0.0f) { sign = -1.0f; }
        }

        n *= sign;

        // 「面までの距離(minDist) + 半径」だけ押し出すと外に出る
        float penetration = (minDist + r);
        Vector3 push = n * penetration;

        outPushForSphere = push;
        outPushForObb = -push;
        return true;
    }

}
