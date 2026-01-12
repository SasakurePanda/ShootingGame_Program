#include "HomingComponent.h"
#include "GameObject.h"
#include "BulletComponent.h"
#include "IScene.h"
#include <DirectXMath.h>
#include <algorithm>
#include <cmath>
#include <iostream>

using namespace DirectX;
using namespace DirectX::SimpleMath;

static bool SolveInterceptTime(const Vector3& relPos,
    const Vector3& targetVel,
    float projSpeed,
    float& outT)
{
    float a = targetVel.Dot(targetVel) - (projSpeed * projSpeed);
    float b = 2.0f * relPos.Dot(targetVel);
    float c = relPos.Dot(relPos);

    if (fabsf(a) < 1e-6f)
    {
        if (fabsf(b) < 1e-6f) 
        {
            return false;
        }
        float t = -c / b;
        if (t > 0.0f)
        {
            outT = t;
            return true;
        }
        return false;
    }

    float disc = b * b - 4.0f * a * c;
    if (disc < 0.0f) 
    {
        return false;
    }

    float sqrtD = std::sqrt(disc);
    float t1 = (-b + sqrtD) / (2.0f * a);
    float t2 = (-b - sqrtD) / (2.0f * a);

    float t = FLT_MAX;
    if (t1 > 0.0f) t = std::min(t, t1);
    if (t2 > 0.0f) t = std::min(t, t2);

    if (t == FLT_MAX) 
    {
        return false; 
    }

    outT = t;
    return true;
}


void HomingComponent::Initialize()
{
    m_age = 0.0f;
}

void HomingComponent::Update(float dt)
{
    // 安全チェック
    auto owner = GetOwner();
    if (!owner) { return; }

    // ライフタイム管理（オプショナル）
    m_age += dt;
    if (m_age >= m_lifeTime)
    {
        IScene* s = owner->GetScene();
        if (s)
        {
            s->RemoveObject(owner);
        }
        return;
    }

    // BulletComponent を取得
    auto bc_sp = owner->GetComponent<BulletComponent>();
    if (!bc_sp) { return; }

    // ターゲット確認
    auto targetSp = m_target.lock();
    if (!targetSp) { return; }

    // 現在の位置・速度ベクトル（ワールド単位）
    Vector3 pos = owner->GetPosition(); // 現在位置
    Vector3 dir = bc_sp->GetVelocity(); // 方向ベクトル（期待は単位ベクトル）
    float speed = bc_sp->GetSpeed();    // スカラー速度
    Vector3 vel = dir * speed;          // 速度ベクトル

    // フォールバックで dir がゼロに近いときは正規化可能な値を作る
    if (dir.LengthSquared() < 1e-6f)
    {
        dir = Vector3::UnitZ;
    }
    else
    {
        dir.Normalize();
    }

    // ターゲットの位置と速度推定
    Vector3 targetPos = targetSp->GetPosition();
    Vector3 targetVel = Vector3::Zero;
    if (m_havePrevTargetPos)
    {
        // 今のターゲット位置 - 前のターゲット位置 / dt で速度を計算
        // dt が 0 に近い場合はガードする
        if (dt > 1e-6f)
        {
            targetVel = (targetPos - m_prevTargetPos) / dt;
        }
    }
    m_prevTargetPos = targetPos;
    m_havePrevTargetPos = true;

    // intercept time を計算（解析解）
    Vector3 rel = targetPos - pos;
    float tIntercept = 0.0f;
    bool haveT = SolveInterceptTime(rel, targetVel, speed, tIntercept);

    float t = 0.0f;
    if (haveT)
    {
        t = tIntercept;
    }
    else
    {
        t = m_timeToIntercept;
        if (t < 0.001f)
        {
            t = 0.001f;
        }
    }

    // 迎撃点（将来の目標位置）
    Vector3 interceptPos = targetPos + targetVel * t;

    // ---------- desiredVec（迎撃方向）計算 ----------
    Vector3 desiredVec = interceptPos - pos;
    if (desiredVec.LengthSquared() < 1e-6f)
    {
        // ほとんどその場にいるなら現在の向きを維持
        desiredVec = dir;
    }

    // ---------- ここで発射時のランダムバイアスを適用 ----------
    // m_aimBias : 単位ベクトル想定（発射側から渡される）
    // m_aimBiasStrength : スカラー（0.0 = 無効, 0..1 = 比率的強さ）
    if (m_aimBiasStrength > 0.0f)
    {
        // 方法A: 単純加算して正規化（見た目が分かりやすい）
        desiredVec = desiredVec + (m_aimBias * m_aimBiasStrength);

        // バイアス強さを時間で減衰させる
        m_aimBiasStrength -= m_aimBiasDecay * dt;
        if (m_aimBiasStrength < 0.0f)
        {
            m_aimBiasStrength = 0.0f;
        }
    }

    // 正規化（以降は方向ベクトルとして使う）
    if (desiredVec.LengthSquared() < 1e-6f)
    {
        desiredVec = dir;
    }
    desiredVec.Normalize();

    // ---------- 角度制限（1フレームあたりの回転上限） ----------
    const float DEG2RAD = 3.14159265358979323846f / 180.0f;
    float maxTurnDeg = m_maxTurnRateDeg;
    if (maxTurnDeg <= 0.0f)
    {
        maxTurnDeg = 60.0f; // デフォルト
    }
    float maxTurnRad = maxTurnDeg * dt * DEG2RAD;

    // 現在方向 dir と desiredVec の角度を求める（数値安全に clamp）
    float dot = dir.Dot(desiredVec);
    if (dot > 1.0f) { dot = 1.0f; }
    if (dot < -1.0f) { dot = -1.0f; }
    float theta = std::acos(dot); // 0..pi

    Vector3 newDir;
    if (theta <= maxTurnRad)
    {
        // 今フレーム内で到達可能なら直接 desiredVec を使う
        newDir = desiredVec;
    }
    else
    {
        // Rodrigues の回転で dir を maxTurnRad だけ回転させる
        Vector3 axis = dir.Cross(desiredVec);
        float axisLen2 = axis.LengthSquared();
        if (axisLen2 < 1e-12f)
        {
            // ほぼ同一直線上 or 逆向き。線形補間で少しだけ寄せる
            float alpha = maxTurnRad / (theta + 1e-6f);
            Vector3 tmp = dir + (desiredVec - dir) * alpha;
            if (tmp.LengthSquared() < 1e-6f)
            {
                tmp = Vector3::UnitZ;
            }
            tmp.Normalize();
            newDir = tmp;
        }
        else
        {
            axis.Normalize();
            float cosT = std::cos(maxTurnRad);
            float sinT = std::sin(maxTurnRad);

            // Rodrigues' rotation: v_rot = v*cosθ + (k × v)*sinθ + k*(k·v)*(1 - cosθ)
            Vector3 term1 = dir * cosT;
            Vector3 term2 = axis.Cross(dir) * sinT;
            Vector3 term3 = axis * (axis.Dot(dir) * (1.0f - cosT));
            Vector3 rotated = term1 + term2 + term3;

            if (rotated.LengthSquared() < 1e-6f)
            {
                rotated = Vector3::UnitZ;
            }
            rotated.Normalize();
            newDir = rotated;
        }
    }

    // 速度の大きさは基本維持（必要ならここで穏やかに補正）
    float newSpeed = speed;

    // 最低速度保証
    float minSpeed = 1e-3f;
    if (newSpeed < minSpeed)
    {
        newSpeed = minSpeed;
    }

    // 書き戻し（BulletComponent 側で位置更新を行う前提）
    bc_sp->SetVelocity(newDir);
    bc_sp->SetSpeed(newSpeed);
}

