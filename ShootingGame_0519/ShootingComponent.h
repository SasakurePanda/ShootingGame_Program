#pragma once
#include "Component.h"
#include "IScene.h"
#include "ICameraViewProvider.h"
#include <memory>
#include <SimpleMath.h>

using namespace DirectX::SimpleMath;

class GameObject;
class Bullet;
class BulletComponent;

class ShootingComponent : public Component
{
public:
    ShootingComponent() = default;
    ~ShootingComponent() override = default;

    //初期化関数
    void Initialize() override {}
    //更新関数
    void Update(float dt) override;

    //シーンとカメラを外部からセット
    void SetScene(IScene* scene) { m_scene = scene; }
    void SetCamera(ICameraViewProvider* camera) { m_camera = camera; }

    //設定用セッター
    void SetCooldown(float cd) { m_cooldown = cd; }
    void SetBulletSpeed(float sp) { m_bulletSpeed = sp; }
    void SetSpawnOffset(float off) { m_spawnOffset = off; }
    const std::vector<std::weak_ptr<GameObject>>& GetSelectedTargets() const { return m_selectedTargets; }
    void SetAutoFire(bool v) { m_autoFire = v; }
    void SetAutoAim(bool v) { m_autoAim = v; }
    void SetMinDistanceToStopShooting(float d) { m_minDistanceToStopShooting = d; }
	void SetAimDistance(float d) { m_aimDistance = d; }

protected:

    //弾生成用関数
    std::shared_ptr<GameObject> CreateBullet(const Vector3& pos, const Vector3& dir);

    //生成した弾を追加する関数
    void AddBulletToScene(const std::shared_ptr<GameObject>& bullet);

private:
    IScene* m_scene = nullptr;                  //生成した弾を追加するシーン
    ICameraViewProvider* m_camera = nullptr;    //発射する向きを取得するカメラ関数

    float m_cooldown = 0.1f;        //クールタイム
    float m_timer = 0.0f;           //経過時間
    float m_bulletSpeed = 220.0f;   //弾の速さ
    float m_spawnOffset = 1.5f;     //発射位置のオフセット
    float m_minDistanceToStopShooting = 0.0f;   //射撃を止める最短距離
    
    bool m_autoFire = false;   //自動発射モード
    bool m_autoAim = false;    //自動照準モード

    std::weak_ptr<GameObject> m_target; //ターゲット保存用配列

    
    float m_aimDistance = 2000.0f;  //スクリーン座標からどれだけ伸ばすかの距離

    std::vector<std::weak_ptr<GameObject>> m_selectedTargets;

    //ホーミングの強さ
    float m_homingStrength = 8.0f;
};


