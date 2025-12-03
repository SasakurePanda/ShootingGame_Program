#include "EnemyAIComponent.h"
#include "GameObject.h"
#include "GameScene.h"
#include "CollisionManager.h"
#include <algorithm>
#include <iostream>
#include <random>

using namespace DirectX::SimpleMath;

EnemyAIComponent::EnemyAIComponent() 
{

}

void EnemyAIComponent::Initialize() 
{
	m_preferredMinY = 5.0f;
	m_preferredMaxY = 30.0f;
	m_baseHeight = (m_preferredMinY + m_preferredMaxY) * 0.5f;

    auto owner = GetOwner();
    if (owner != nullptr)
    {
        m_lastPos = owner->GetPosition();
    }
    m_stuckTimer = 0.0f;
    m_inEscapeMode = false;
    m_escapeTime = 0.0f;
}

Vector3 EnemyAIComponent::ComputeFlee(const Vector3& pos)
{
	//プレイヤーオブジェクトが存在しないなら
    if(m_target == nullptr)
    {
        return Vector3::Zero;
    }

	//プレイヤーから逃げる方向を計算(ターゲットPos - 自身のPos)
    Vector3 toPlayer = m_target->GetPosition() - pos;
    float dist2 = toPlayer.LengthSquared();
    if (dist2 < 1e-6f)
    {
        return Vector3::Zero;
    }

	//逃げる方向をXZだけでまず作る
    Vector3 PlayerXZ(toPlayer.x,0.0f,toPlayer.z);
	if (PlayerXZ.LengthSquared() < 1e-6f)
    {
        Vector3 fallbackDir = Vector3(-1.0f, 0.0f, 0.0f);
        fallbackDir.Normalize();
        return fallbackDir * m_fleeStrength;
    }

    //逃げる方向を正規化して強さを掛ける
    Vector3 fleeXZ = -PlayerXZ;
    fleeXZ.Normalize();


    float vertical = 0.0f;

    //プレイヤーがかなり上下に離れているときだけ少しY成分を足す
    float dy = toPlayer.y;
    float absDy = fabsf(dy);
    float verticalThreshold = 10.0f;

	//指定した閾値よりも上下の差が大きい場合
    if (absDy > verticalThreshold)
    {
      
        //敵からプレイヤーへ向かうYベクトルが
		//正ならプレイヤーは上、負なら下にいる
        if (dy > 0.0f) 
        {
            vertical = m_verticalFleeScale;
        }
        else
        {
            vertical = -m_verticalFleeScale;
        }
    }

    Vector3 fleeDir(fleeXZ.x, vertical, fleeXZ.z);
    fleeDir.Normalize();

    return fleeDir * m_fleeStrength;
}

Vector3 EnemyAIComponent::ComputeFleeVelocity(const Vector3& pos) const
{
	//ターゲットオブジェクトが存在しないなら
    if (!m_target)
    {
        return Vector3::Zero;
    }

	//ターゲットから逃げる方向を計算
	Vector3 toPlayer = m_target->GetPosition() - pos;   //ターゲットの位置 - 自身の位置

	//ターゲットと同じ位置にいる場合は逃げられない
    if (toPlayer.LengthSquared() < 1e-6f)
    {
        return Vector3::Zero;
    }

    //XZ平面だけで逃げ方向を決める
    Vector3 toPlayerXZ(toPlayer.x, 0.0f, toPlayer.z);
    if (toPlayerXZ.LengthSquared() < 1e-6f)
    {
         return Vector3::Zero;
    }

    toPlayerXZ.Normalize();

    //プレイヤーと逆向き
    Vector3 fleeDir = -toPlayerXZ;

    //ここではYを0にしておく
    Vector3 fleeVel(fleeDir.x, 0.0f, fleeDir.z);

	//速度ベクトル = 逃げ方向 × 最大速度
    return fleeVel * m_maxSpeed;
}

Vector3 EnemyAIComponent::ComputeAvoidance(const Vector3& pos, const Vector3& forward)
{
    Vector3 avoid = Vector3::Zero;

    auto owner = GetOwner();
    if (!owner)
    {
        return avoid;
    }

    auto scene = dynamic_cast<GameScene*>(owner->GetScene());
    if (!scene)
    {
        return avoid;
    }

    // 水平前方ベクトル（XZ）を作っておく
    Vector3 forwardXZ(forward.x, 0.0f, forward.z);
    if (forwardXZ.LengthSquared() < 1e-6f)
    {
        forwardXZ = Vector3::Forward; // フォールバック
    }
    forwardXZ.Normalize();

    for (int i = 0; i < m_feelerCount; ++i)
    {
        float t;
        if (m_feelerCount == 1)
        {
            t = 0.0f;
        }
        else
        {
            t = float(i) / (m_feelerCount - 1);
        }

        float angle = (t - 0.5f) * 2.0f * m_feelerSpread;

        // Y 軸回りに回転（水平面内でのみ扇状）
        Matrix rot = Matrix::CreateRotationY(angle);
        Vector3 dir = Vector3::Transform(forwardXZ, rot);
        dir.Normalize();

        float len = m_lookahead * (1.0f - (std::abs(angle) / m_feelerSpread));

        RaycastHit hit;
        bool collided = scene->RaycastForAI(pos, dir, len, hit, owner);

        if (collided)
        {
            float factor = (len - hit.distance) / len;

            // --- 法線方向の押し返し（水平成分だけ） ---
            Vector3 n = hit.normal;
            Vector3 nXZ(n.x, 0.0f, n.z);
            if (nXZ.LengthSquared() < 1e-6f)
            {
                nXZ = -dir; // 万が一真上/真下とかなら、とりあえず後ろへ
            }
            nXZ.Normalize();

            Vector3 push = nXZ * factor * 2.0f;

            // --- 左右方向（水平な side ベクトル） ---
            Vector3 side = forwardXZ.Cross(Vector3::Up); // 水平な横方向 (-Z,0,X)
            if (side.LengthSquared() > 1e-6f)
            {
                side.Normalize();
                // 建物から少し「横にずれる」成分を足す
                push += side * factor * 1.5f;
            }

            // 最後に Y 成分は 0 に固定（上下回避はここではしない）
            push.y = 0.0f;

            avoid += push;
        }
    }

    return avoid;
}


Vector3 EnemyAIComponent::ComputeBoundaryAvoid(const Vector3& pos) const
{
    if (!m_playArea)
    {
        return Vector3::Zero;
    }

    const Vector3& minB = m_playArea->GetBoundsMin();
    const Vector3& maxB = m_playArea->GetBoundsMax();

    Vector3 avoid = Vector3::Zero;

    //X方向の壁
    float distLeft = pos.x - minB.x;    //左壁までの距離
    float distRight = maxB.x - pos.x;   //右壁までの距離

    //左壁との距離が近づいたとみなす距離よりも小さいか
    //短ければ押し戻す力を加える
    if(distLeft < m_boundaryMargin)
    {
        float t = 1.0f - distLeft / m_boundaryMargin; 
        avoid += Vector3(1, 0, 0) * t;                  
    }

    //右壁との距離が近づいたとみなす距離よりも小さいか
	//短ければ押し戻す力を加える
    if(distRight < m_boundaryMargin)
    {
        float t = 1.0f - distRight / m_boundaryMargin;
        avoid += Vector3(-1, 0, 0) * t;
    }

    //Z方向の壁
    float distBack = pos.z - minB.z;    //奥  (-Z 側)まで
    float distFront = maxB.z - pos.z;   //手前(+Z 側)まで

    //奥(-Z 側)との距離が近づいたとみなす距離よりも小さいか
	//短ければ押し戻す力を加える
    if (distBack < m_boundaryMargin)
    {
        float t = 1.0f - distBack / m_boundaryMargin;
        avoid += Vector3(0, 0, 1) * t;   //+Z側(内側)へ押す
    }

	//手前(+Z 側)との距離が近づいたとみなす距離よりも小さいか
    //短ければ押し戻す力を加える 
    if (distFront < m_boundaryMargin)
    {
        float t = 1.0f - distFront / m_boundaryMargin;
        avoid += Vector3(0, 0, -1) * t;  //-Z側(内側)へ押す
    }

    //今はYを無視（空を飛ぶ高さ一定なら要らない）
    //おそらく今後追加する項目になる

    return avoid;
}

Vector3 EnemyAIComponent::ComputeHeightControl(const Vector3& pos) const
{

    Vector3 steer = Vector3::Zero;

	//指定している範囲に収まっているかどうか
    //指定している範囲よりも下にいる場合
    if (pos.y < m_preferredMinY)
    {   
        //どれだけ下にいるか
        float diff = m_preferredMinY - pos.y;
        //正規化(0～1にスケールする)
        float t = diff / 30.0f;               
        if (t > 1.0f)
        {
            t = 1.0f;
        }
        steer.y = t * m_heightSteerStrength;    //上向きに力をかける 
    }
    //指定している範囲よりも上にいる場合
    else if (pos.y > m_preferredMaxY)
    {
		float diff = pos.y - m_preferredMaxY;
        float t = diff / 30.0f;
        if (t > 1.0f)
        {
            t = 1.0f;
        }
        steer.y = -t * m_heightSteerStrength;  //下向きに力をかける 
    }
	//指定範囲内にいる場合
    else
	{
        float diff = pos.y - m_baseHeight;
		float absDiff = fabsf(diff);
		float deadZone = 3.0f; 

        if (absDiff > deadZone)
        {
			float t = absDiff / (m_preferredMaxY - m_preferredMinY);
			if (t > 1.0f)
            {
                t = 1.0f;
            }
            if (diff > 0.0f)
            {
				steer.y = -t * 0.5f * m_heightSteerStrength; //下向きに力をかける
            }
            else
            {
                steer.y = t * 0.5f * m_heightSteerStrength;
            }
        }
    }
	return steer;
}

void EnemyAIComponent::Update(float dt)
{
    auto owner = GetOwner();

	//オーナーとdtチェック
    if (!owner || dt <= 0) { return; }

	//現在位置と前方ベクトルを取得
    Vector3 pos = owner->GetPosition();

    //Enemyの前方ベクトル(forwardはワールド前方)
    Vector3 forward = owner->GetForward();
    forward.Normalize();

    Vector3 desiredFleeVel = ComputeFleeVelocity(pos);
	//ターゲットから逃げる力を計算
    //Vector3 flee = ComputeFlee(pos);
	//周りの障害物回避力を計算
    Vector3 avoid = ComputeAvoidance(pos, forward);
    //境界線回避
    Vector3 boundary = ComputeBoundaryAvoid(pos);
    //
    Vector3 height = ComputeHeightControl(pos);

	//ジッターを加えて不規則に
    static std::mt19937 rng(1234);
    static std::uniform_real_distribution<float> d(-0.2f, 0.2f);
    Vector3 jitter(d(rng), d(rng) * 0.2f, d(rng));

	//ターゲットから避ける力と障害物回避と境界線回避の力を合成して
    //方向を決める
    Vector3 steeringFromForces = Vector3::Zero;
    steeringFromForces += avoid    * m_maxSpeed * 0.7f;
    steeringFromForces += boundary * m_maxSpeed * 0.8f;
    steeringFromForces += height   * m_maxSpeed * 0.2f;   // 高さ制御
    steeringFromForces += jitter   * 2.0f;

    Vector3 desiredVel = desiredFleeVel + steeringFromForces;

    Vector3 toPlayer = Vector3::Zero;
    if (m_target != nullptr)
    {
        toPlayer = m_target->GetPosition() - pos;
    }

    if (toPlayer.LengthSquared() > 1e-6f)
    {
        toPlayer.Normalize();

        // desiredVel の中で「プレイヤーへ向かう成分」がどれだけあるか
        float towardsPlayer = desiredVel.Dot(toPlayer);

        // もし 0 より大きければ、プレイヤーに少しでも近づこうとしている
        if (towardsPlayer > 0.0f)
        {
            // その成分を引き算して、「プレイヤーへ戻る」要素を削る
            Vector3 correction = toPlayer * towardsPlayer;
            desiredVel = desiredVel - correction;
        }
    }

    if (desiredVel.LengthSquared() < 1e-6f)
    {
        // forward と Up から横方向を作る
        Vector3 side = forward.Cross(Vector3::Up);
        if (side.LengthSquared() < 1e-6f)
        {
            side = Vector3::Right;
        }
        side.Normalize();
        desiredVel = side * m_maxSpeed * 0.5f;
    }


    Vector3 steer = desiredVel - m_velocity;

    //ベクトルの長さ取得
    float steerLen = steer.Length();

    //1フレーム当たりの最大力を超えないように制限
    if (steerLen > m_maxForce)
    {
        steer = steer * (m_maxForce / steerLen);
    }

    //速度更新
    m_velocity += steer * dt;

    //スピードのベクトルの長さを計算
    float speed = m_velocity.Length();

    //最大速度を超えないように制限
    if (speed > m_maxSpeed)
    {
        m_velocity *= (m_maxSpeed / speed);
    }

    // 位置の候補
    Vector3 desiredPos = pos + m_velocity * dt;

    if (m_playArea)
    {
        // PlayArea のルールに従って補正
        Vector3 resolved = m_playArea->ResolvePosition(pos, desiredPos, owner);

        // もし壁で止められたなら、速度も整合を取る（次フレームの暴れ防止）
        Vector3 delta = resolved - pos;
        if (dt > 0.0f)
        {
            // 実際に動けた距離から「有効速度」を再計算
            m_velocity = delta / dt;
        }

        owner->SetPosition(resolved);
    }
    else
    {
        owner->SetPosition(desiredPos);
    }

    Vector3 currentPos = owner->GetPosition();

    // このフレームでどれだけ動けたか
    float movedDist = (currentPos - m_lastPos).Length();
    if (movedDist < 0.5f)
    {
        m_stuckTimer += dt;
    }
    else
    {
        m_stuckTimer = 0.0f;
    }

    m_lastPos = currentPos;

    // 一定時間以上スタックしたら「脱出モード」開始
    if (m_stuckTimer > 0.7f && !m_inEscapeMode)
    {
        m_inEscapeMode = true;
        m_escapeTime = 0.6f;  // 0.6秒だけ強制脱出

        Vector3 dir = Vector3::Zero;

        if (m_target != nullptr)
        {
            Vector3 toP = m_target->GetPosition() - currentPos;
            if (toP.LengthSquared() > 1e-6f)
            {
                toP.Normalize();
                // 上方向と toPlayer で外積をとると「横方向」が出る
                dir = Vector3::Up.Cross(toP);
            }
        }

        if (dir.LengthSquared() < 1e-6f)
        {
            dir = Vector3::Right;
        }
        dir.Normalize();

        //少しだけ上にも逃がす
        //dir.y = 0.4f;
        dir.y = 0.0f;
        dir.Normalize();

        m_escapeDir = dir;
    }

    if (m_inEscapeMode)
    {
        m_escapeTime -= dt;
        if (m_escapeTime <= 0.0f)
        {
            m_inEscapeMode = false;
        }
        else
        {
            // 次フレーム用の速度を「脱出用ベクトル」に固定
            m_velocity = m_escapeDir * m_maxSpeed;
        }
    }

    //回転更新：速度ベクトルに基づく
    if (m_velocity.LengthSquared() > 1e-6f)
    {
        float yaw = std::atan2(m_velocity.x, m_velocity.z);
        float pitch = std::asin(std::clamp(m_velocity.y / m_velocity.Length(), -1.0f, 1.0f));

        static Vector3 prevForward = forward;
        Vector3 currForward = m_velocity;
        currForward.Normalize();
        float yawDot = prevForward.Dot(currForward);
        float turnAmount = 1.0f - yawDot; // 0..2
        float roll = -turnAmount * 0.8f;
        prevForward = currForward;

        Vector3 rot = owner->GetRotation();
        rot.y = rot.y * 0.9f + yaw * 0.1f;
        rot.x = rot.x * 0.9f + pitch * 0.1f;
        rot.z = rot.z * 0.9f + roll * 0.1f;
        owner->SetRotation(rot);
    }

    /*std::cout << "EnemyAIComponent::Update Position: (" 
              << currentPos.x << ", " 
              << currentPos.y << ", " 
		<< currentPos.z << ")\n";*/
}