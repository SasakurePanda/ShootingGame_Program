#include "FixedTurretComponent.h"
#include "SceneManager.h"
#include "Bullet.h"
#include <iostream>

void FixedTurretComponent::Initialize()
{
    m_timer = 0.0f;
}

void FixedTurretComponent::Update(float dt)
{
    if (!GetOwner()) { return; }

    auto sp = m_target.lock();
    if (!sp) 
    {
        std::cout << "ターゲットが見つからないため早期リターンします" << std::endl;
        return;
    }

    Vector3 myPos = GetOwner()->GetPosition();
    Vector3 targetPos = sp->GetPosition();

    Vector3 toTarget = targetPos - myPos;

    Vector3 horiz = targetPos;
    horiz.y = 0.0f;
    float horizLenSq = horiz.LengthSquared();  

    //Y軸だけ回転（水平面で向く）
    if (horizLenSq > 1e-6f)
    {
        horiz.Normalize();   //正規化
        float yaw = atan2(toTarget.x, toTarget.z);
        Vector3 rot = GetOwner()->GetRotation();
        rot.y = yaw;
        GetOwner()->SetRotation(rot);
    }
    else
    {
        std::cout << "Playerが真上又は真下の為、変更しません" << std::endl;

    }

    Vector3 shootDir = targetPos - (myPos + Vector3(0.0f, 1.0f, 0.0f)); // 少し頭上めがける
    float shootLenSq = shootDir.LengthSquared();
    if (shootLenSq > 1e-6f)
    {
        shootDir.Normalize();
    }
    
    // 射撃タイマー
    m_timer += dt;

    std::cout << "Turret Update : timer = " << dt << std::endl;
   
    if (m_timer >= m_cooldown)
    {
        if (shootLenSq > 1e-6f)
        {
            Shoot(shootDir);
            m_timer = 0.0f;
        }
        else
        {
            std::cout << "shootLenSqが短い為、Shoot関数には入らずに終了します" << std::endl;
        }
    }
}

void FixedTurretComponent::Shoot(const Vector3& dir)
{
    auto owner = GetOwner();
    if (!owner) { return; }

    //dirが極端に小さくないかチェック
    if (dir.LengthSquared() <= 1e-6f)
    {
        std::cout << "FixedTurretComponent::Shoot - dir is zero, abort"<< std::endl;
    }

    Vector3 nd = dir;
    nd.Normalize(); //正規化

    std::cout << "Shootが呼び出されました、生成を始めます" << std::endl;

    auto bullet = std::make_shared<Bullet>();
    bullet->SetPosition(owner->GetPosition() + Vector3(0, 1.0f, 0)); // 少し上から発射
    auto bc = std::make_shared<BulletComponent>();
    bc->SetVelocity(dir);
    bc->SetSpeed(m_bulletSpeed);
    bc->SetBulletType(BulletComponent::ENEMY);
    bullet->AddComponent(bc);
    bullet->Initialize();

    if (auto scene = owner->GetScene())
    {
        scene->AddObject(bullet);
    }
    else
    {
        std::cout << "Sceneを取得できませんでした。追加せずに終了します"<< std::endl;
    }
}