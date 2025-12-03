#pragma once
#include "Component.h"
#include <SimpleMath.h>
#include <memory> 

using namespace DirectX::SimpleMath;

class GameObject; // 前方宣言

class BulletComponent : public Component
{
public: 
    //弾の種類のENUM
    enum BulletType
    {
        UNKNOW,
        PLAYER,
        ENEMY
    };

    BulletComponent() = default;
    ~BulletComponent() override = default;

    //初期化関数
    void Initialize() override;

    //更新関数
    void Update(float dt) override;

    //外部(主にShootingComponentからかな)から決めるためのセッター関数
    //方向ベクトル
    void SetVelocity(const Vector3& v) { m_velocity = v; }

    //スピード
    void SetSpeed(float s) { m_speed = s; }

    //弾の生存時間
    void SetLifetime(float sec) { m_lifetime = sec; }

    //弾の種類
    void SetBulletType(BulletType t) { m_ownerType = t; }
    BulletType GetBulletType() const { return m_ownerType; }

    // ホーミングターゲットを受け取る（weak_ptr で保持）
    void SetTarget(std::weak_ptr<GameObject> t) { m_target = t; }
    std::weak_ptr<GameObject> GetTarget() const { return m_target; }

    // ホーミング強度（1 = ダイレクトで即追尾、0 = 追尾なし）
    void SetHomingStrength(float s) { m_homingStrength = s; }

private:
    Vector3 m_velocity = Vector3::Zero; //方向ベクトル(単位ベクトルが望ましい)
    float m_speed = 40.0f;              //スピード
    float m_age = 0.0f;                 //経過時間
    float m_lifetime = 3.0f;            //生存時間
    
    //弾の種類
    BulletType m_ownerType = BulletType::UNKNOW;  

    std::weak_ptr<GameObject> m_target; //追尾相手
    float m_homingStrength = 8.0f; //追尾の力の強さ
    
};

