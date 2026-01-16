#include "EnemySpawner.h"
#include "GameScene.h"
#include "Enemy.h"
#include "ModelComponent.h"
#include "OBBColliderComponent.h"
#include "PatrolComponent.h"
#include "CircularPatrolComponent.h"
#include "FixedTurretComponent.h"
#include "EnemyAIComponent.h"
#include "HitPointCompornent.h"
#include "PushOutComponent.h"
#include "SphereColliderComponent.h"

EnemySpawner::EnemySpawner(GameScene* scene) : m_scene(scene)
{
    patrolCfg.waypoints = 
    {
        {0,0,0}, {100,0,0}, {100,0,100}, {0,0,100}
    };
}

/// <summary>
/// 決めた地点を動く敵のスポーン用関数
/// </summary>
/// <param name="cfg">敵の設定の入れたコンフィグ</param>
/// <param name="pos">敵のスポーン位置</param>
/// <returns></returns>
std::shared_ptr<GameObject> EnemySpawner::SpawnPatrolEnemy(const PatrolConfig& cfg, 
                                                           const DirectX::SimpleMath::Vector3& pos)
{
    //Enemyを生成し、初期設定を行う
    auto enemy = std::make_shared<Enemy>();
    enemy->SetScene(m_scene);
    enemy->SetPosition(pos);
    enemy->SetScale({ 4.0f, 4.0f, 4.0f });

    //モデルの設定を行い、Componentを付ける
    auto model = std::make_shared<ModelComponent>();
    model->LoadModel("Asset/Model/Enemy/EnemyFighterjet.obj");
    enemy->AddComponent(model);

    //PatrolEnemyの設定を行い、Componentを付ける
    auto patrol = std::make_shared<PatrolComponent>();
    if (!cfg.waypoints.empty())
    {
        patrol->SetWaypoints(cfg.waypoints);
    }
    patrol->SetSpeed(cfg.speed);
    patrol->SetArrivalThreshold(cfg.arrival);
    patrol->SetPingPong(cfg.pingPong);

    //スプライン用の設定追加
    patrol->SetUseSpline(true);
    patrol->SetSplineTension(5.0f);
    patrol->SetLoop(true);
    patrol->SetFaceMovement(true);

    //コンポーネント追加(敵の動き)
    enemy->AddComponent(patrol);

    //HP設定
    auto hp = std::make_shared<HitPointComponent>(1);
    hp->SetInvincibilityOnHit(0.0f);

    //コンポーネント追加(HP)
    enemy->AddComponent(hp);

    //当たり判定の設定を行い、Componentを付ける
    auto col = std::make_shared<SphereColliderComponent>();
    col->SetRadius(7.5f);                       // 半径。サイズ感に合わせて調整
    col->SetLocalOffset(Vector3(0.0f, 0.0f, 0.0f)); // 見た目中心が上ならオフセット
    enemy->AddComponent(col);

    auto push = std::make_shared<PushOutComponent>();
    push->SetMass(5.0f);
    enemy->AddComponent(push);
   
    //初期化
    enemy->Initialize();

    //シーンに登録
    m_scene->AddObject(enemy); 

    return enemy;
}

std::shared_ptr<GameObject> EnemySpawner::SpawnCircleEnemy(const CircleConfig& cfg, const DirectX::SimpleMath::Vector3& pos)
{
    //Enemyを生成し、初期設定を行う
    auto enemy = std::make_shared<Enemy>();
    enemy->SetScene(m_scene);
    enemy->SetPosition(pos);

    enemy->SetInitialHP(3);

    //モデルの設定を行い、Componentを付ける
    auto model = std::make_shared<ModelComponent>();
    model->LoadModel("Asset/Model/Enemy/EnemyFighterjet.obj");
    enemy->AddComponent(model);

    //当たり判定の設定を行い、Componentを付ける
    auto col = std::make_shared<OBBColliderComponent>();
    col->SetSize({ 3,3,3 });
    enemy->AddComponent(col);

    //CirculPatrolEnemyの設定を行い、Componentを付ける
    auto circ = std::make_shared<CirculPatrolComponent>();
    circ->SetCenter(cfg.center);
    circ->SetRadius(cfg.radius);
    circ->SetAngularSpeed(cfg.angularSpeed);
    circ->SetClockwise(cfg.clockwise);
    enemy->AddComponent(circ);

    //HP設定
    auto hp = std::make_shared<HitPointComponent>(1);
    hp->SetInvincibilityOnHit(0.0f);

    //コンポーネント追加(HP)
    enemy->AddComponent(hp);

    auto push = std::make_shared<PushOutComponent>();
    enemy->AddComponent(push);

    m_scene->AddObject(enemy);

    return enemy;
}

std::shared_ptr<GameObject> EnemySpawner::SpawnTurretEnemy(const TurretConfig& cfg, const DirectX::SimpleMath::Vector3& pos)
{
    //Enemyを生成し、初期設定を行う
    auto enemy = std::make_shared<Enemy>();
    enemy->SetScene(m_scene);
    enemy->SetPosition(pos);
    enemy->SetInitialHP(3);

    //モデルの設定を行い、Componentを付ける
    auto model = std::make_shared<ModelComponent>();
    model->LoadModel("Asset/Model/Enemy/EnemyFighterjet.obj");
    enemy->AddComponent(model);

    //当たり判定の設定を行い、Componentを付ける
    auto col = std::make_shared<AABBColliderComponent>();
    col->SetSize({ 3,3,3 });
    enemy->AddComponent(col);

    //TurretEnemyの設定を行い、Componentを付ける
    auto turt = std::make_shared<FixedTurretComponent>();
    turt->SetCooldown(turretCfg.coolTime);
    turt->SetBulletSpeed(turretCfg.bulletSpeed);
    turt->SetTarget(turretCfg.target);
    enemy->AddComponent(turt);

    //HP設定
    auto hp = std::make_shared<HitPointComponent>(1);
    hp->SetInvincibilityOnHit(0.0f);

    //コンポーネント追加(HP)
    enemy->AddComponent(hp);

    auto push = std::make_shared<PushOutComponent>();
    push->SetMass(15.0f);
    enemy->AddComponent(push);

    m_scene->AddObject(enemy);

    return enemy;
}

std::shared_ptr<GameObject> EnemySpawner::SpawnFleeEnemy(const FleeConfig& cfg,
    const DirectX::SimpleMath::Vector3& pos)
{
    using namespace DirectX::SimpleMath;

    //Enemyを生成し、初期設定を行う
    auto enemy = std::make_shared<Enemy>();
    enemy->SetScene(m_scene);
    enemy->SetPosition(pos);
    enemy->SetScale({ 3.0f, 3.0f, 3.0f });
    enemy->SetInitialHP(3);
    enemy->SetBoundingRadius(3.0f); //Raycast用の半径（GameScene::Raycast が使う）

    //モデル
    auto model = std::make_shared<ModelComponent>();
    model->LoadModel("Asset/Model/Enemy/EnemyFighterjet.obj");
    enemy->AddComponent(model);

    //コライダー
    auto col = std::make_shared<OBBColliderComponent>();
    col->SetSize({ 3,3,3 });
    enemy->AddComponent(col);

    //AIコンポーネント
    auto ai = std::make_shared<EnemyAIComponent>();
    ai->SetMaxSpeed(cfg.maxSpeed);
    ai->SetMaxForce(cfg.maxForce);
    ai->SetFleeStrength(cfg.fleeStrength);
    ai->SetLookahead(cfg.lookahead);
    ai->SetFeelerCount(cfg.feelerCount);
    ai->SetFeelerSpread(cfg.feelerSpread);

    if (auto p = cfg.player.lock())
    {
        ai->SetTarget(p.get());
    }

    //PlayArea を渡す
    if (m_scene && m_scene->GetPlayArea()) 
    {
        ai->SetPlayArea(m_scene->GetPlayArea());
    }

    enemy->AddComponent(ai);

    enemy->Initialize();

    m_scene->AddObject(enemy);

    return enemy;
}

void EnemySpawner::EnsurePatrolCount()
{
    //今生きているweak_ptrを参照に変換し整理
    std::vector<std::shared_ptr<GameObject>> live;

    for (auto& w : m_spawnedPatrols) 
    {
        //このptrが生きているかどうか
        if (auto sp = w.lock()) 
        {
            live.push_back(sp);
        }
    }

    //空にする
    m_spawnedPatrols.clear();

    for (auto& s : live)
    {
        //起動しているやつを配列に戻す
        m_spawnedPatrols.push_back(s);
    }

    int current = (int)live.size();
    int want = patrolCfg.spawnCount;

    //今の生成数が指定数より少ないなら
    if (current < want) 
    {
        int baseIndex = current;

        for (int i = 0; i < want - current; ++i)
        {
            int slot = baseIndex + i;
            //サイクルしてウェイポイントセットを選ぶ（セット数が 2 個なら 0/1/0/1...）
            const auto& selectedWaypoints = patrolWaypointSets[slot % patrolWaypointSets.size()];

            PatrolConfig localCfg = patrolCfg;           //コピーして編集
            localCfg.waypoints = selectedWaypoints;      //この敵用にウェイポイントをセット

            // spawnPos は適宜決める。最初のウェイポイント上に置きたいなら:
            DirectX::SimpleMath::Vector3 spawnPos = selectedWaypoints.front();

            auto e = SpawnPatrolEnemy(localCfg, spawnPos);
            m_spawnedPatrols.push_back(e);
        }
    }
    else if (current > want)
    {
        // 余剰分を削除（末尾から）
        int removeCount = current - want;
        for (int i = 0; i < removeCount; ++i)
        {
            if (auto sp = m_spawnedPatrols.back().lock()) 
            {
                // シーンの削除キューへ入れる（FinishFrameCleanup と合わせる）
                m_scene->m_DeleteObjects.push_back(sp); // 仮 API。なければ m_scene->m_DeleteObjects.push_back(sp) 相当を行う
            }
            m_spawnedPatrols.pop_back();
        }
    }
}

void EnemySpawner::EnsureTurretCount()
{
    //今生きているweak_ptrを参照に変換し整理
    std::vector<std::shared_ptr<GameObject>> live;

    for (auto& w : m_spawnedTurrets)
    {
        //このptrが生きているかどうか
        if (auto sp = w.lock()) 
        {
            live.push_back(sp);
        }
    }

    //空にする
    m_spawnedTurrets.clear();

    for (auto& s : live)
    {
        //起動しているやつを配列に戻す
        m_spawnedTurrets.push_back(s);
    }

    int current = (int)live.size();
    int want = turretCfg.spawnCount;

    //今の生成数が指定数より少ないなら
    if (current < want) 
    {
        int baseIndex = current;

        for (int i = 0; i < want - current; ++i)
        {
            int slot = baseIndex + i;

            //サイクルしてウェイポイントセットを選ぶ
            const auto& selectedPos = TurretPosSets[slot % TurretPosSets.size()];

            TurretConfig localCfg = turretCfg;           //コピーして編集
            localCfg.pos = selectedPos;      //この敵用にウェイポイントをセット

            auto e = SpawnTurretEnemy(localCfg, localCfg.pos);
            m_spawnedTurrets.push_back(e);
        }
    }
    else if (current > want)
    {
        // 余剰分を削除（末尾から）
        int removeCount = current - want;
        for (int i = 0; i < removeCount; ++i)
        {
            if (auto sp = m_spawnedTurrets.back().lock())
            {
                // シーンの削除キューへ入れる（FinishFrameCleanup と合わせる）
                m_scene->m_DeleteObjects.push_back(sp); // 仮 API。なければ m_scene->m_DeleteObjects.push_back(sp) 相当を行う
            }
            m_spawnedTurrets.pop_back();
        }
    }
}

void EnemySpawner::EnsureCircleCount()
{
    //今生きているweak_ptrを参照に変換し整理
    std::vector<std::shared_ptr<GameObject>> live;
    for (auto& w : m_spawnedCircles)
    {
        //このptrが生きているかどうか
        if (auto sp = w.lock())
        {
            live.push_back(sp);
        }
    }
    
    //空にする
    m_spawnedCircles.clear();

    for (auto& s : live)
    {
        //起動しているやつを配列に戻す
        m_spawnedCircles.push_back(s);
    }


    int current = (int)live.size();
    int want = circleCfg.spawnCount;

    if (current < want)
    {
        int baseIndex = current;

        //円周上に配置
        for (int i = 0; i < want - current; ++i)
        {
            int slot = baseIndex + i;

            int idx = current + i;

            CircleConfig localCfg = circleCfg;           //コピーして編集

            //半径を選ぶ（セット数が 2 個なら 0/1/0/1...）
            const auto& selectedRadius = circlePatrolRadiusSets[slot % patrolWaypointSets.size()];
            localCfg.radius = selectedRadius;

            //中心座標を選ぶ（セット数が 2 個なら 0/1/0/1...）
            const auto& selectedCenter = circlePatrolCenterSets[slot % patrolWaypointSets.size()];
            localCfg.center = selectedCenter;

            float ang = (2.0f * DirectX::XM_PI * idx) / want;
            float x = circleCfg.center.x + cosf(ang) * circleCfg.radius;
            float z = circleCfg.center.z + sinf(ang) * circleCfg.radius;

            auto e = SpawnCircleEnemy(localCfg, { x, 0.0f, z });
            m_spawnedCircles.push_back(e);
        }
    }
    else if (current > want)
    {
        int removeCount = current - want;
        for (int i = 0; i < removeCount; ++i)
        {
            if (auto sp = m_spawnedCircles.back().lock()) 
            {
                m_scene->m_DeleteObjects.push_back(sp);
            }
            m_spawnedCircles.pop_back();
        }
    }
}

void EnemySpawner::EnsureFleeCount()
{
    // 今生きている weak_ptr を整理
    std::vector<std::shared_ptr<GameObject>> live;
    for (auto& w : m_spawnedFlees)
    {
        if (auto sp = w.lock())
        {
            live.push_back(sp);
        }
    }

    m_spawnedFlees.clear();
    for (auto& s : live)
    {
        m_spawnedFlees.push_back(s);
    }

    int current = static_cast<int>(live.size());
    int want = fleeCfg.spawnCount;

    if (current < want)
    {
        int baseIndex = current;

        for (int i = 0; i < want - current; ++i)
        {
            int idx = baseIndex + i;

            // とりあえず適当なスポーン位置（原点周りに円形配置）
            DirectX::SimpleMath::Vector3 center = { 0.0f, 3.0f, 0.0f };
            float radius = 50.0f;
            float ang = (2.0f * DirectX::XM_PI * idx) / max(want, 1);
            float x = center.x + cosf(ang) * radius;
            float z = center.z + sinf(ang) * radius;
            DirectX::SimpleMath::Vector3 spawnPos(x, center.y, z);

            auto e = SpawnFleeEnemy(fleeCfg, {0.0f,20.0f,0.0f});
            m_spawnedFlees.push_back(e);
        }
    }
    else if (current > want)
    {
        int removeCount = current - want;
        for (int i = 0; i < removeCount; ++i)
        {
            if (auto sp = m_spawnedFlees.back().lock())
            {
                m_scene->m_DeleteObjects.push_back(sp);
            }
            m_spawnedFlees.pop_back();
        }
    }
}


void EnemySpawner::ApplyPatrolSettingsToAll()
{
    for (auto& w : m_spawnedPatrols)
    {
        //今このptrが生きていたら
        if (auto sp = w.lock())
        {
            //そのptrからComponentを取ってきて
            auto patrol = sp->GetComponent<PatrolComponent>();
            if (patrol)
            {
                //あったら現段階での設定をする
                patrol->SetWaypoints(patrolCfg.waypoints);
                patrol->SetSpeed(patrolCfg.speed);
                patrol->SetArrivalThreshold(patrolCfg.arrival);
                patrol->SetPingPong(patrolCfg.pingPong);
            }
        }
    }
}

void EnemySpawner::ApplyCircleSettingsToAll()
{
    for (auto& w : m_spawnedCircles)
    {
        //今このptrが生きていたら
        if (auto sp = w.lock()) 
        {
            //そのptrからComponentを取ってきて
            auto circ = sp->GetComponent<CirculPatrolComponent>();
            if (circ) 
            {
                //あったら現段階での設定をする
                circ->SetCenter(circleCfg.center);
                circ->SetRadius(circleCfg.radius);
                circ->SetAngularSpeed(circleCfg.angularSpeed);
                circ->SetClockwise(circleCfg.clockwise);
            }
        }
    }
}

void EnemySpawner::ApplyTurretSettingsToAll()
{
    for (auto& w : m_spawnedTurrets)
    {
        //今このptrが生きていたら
        if (auto sp = w.lock()) 
        {
            //そのptrからComponentを取ってきて
            auto turt = sp->GetComponent<FixedTurretComponent>();
            if (turt)
            {
                turt->SetCooldown(turretCfg.coolTime);
                turt->SetBulletSpeed(turretCfg.bulletSpeed);
            }
        }
    }
}

void EnemySpawner::ApplyFleeSettingsToAll()
{
    for (auto& w : m_spawnedFlees)
    {
        if (auto sp = w.lock())
        {
            auto ai = sp->GetComponent<EnemyAIComponent>();
            if (ai)
            {
                ai->SetMaxSpeed(fleeCfg.maxSpeed);
                ai->SetMaxForce(fleeCfg.maxForce);
                ai->SetFleeStrength(fleeCfg.fleeStrength);
                ai->SetLookahead(fleeCfg.lookahead);
                ai->SetFeelerCount(fleeCfg.feelerCount);
                ai->SetFeelerSpread(fleeCfg.feelerSpread);

                if (auto p = fleeCfg.player.lock())
                {
                    ai->SetTarget(p.get());
                }
            }
        }
    }
}


void EnemySpawner::DestroyAll()
{
    for (auto& w : m_spawnedPatrols)
    {
        //今このptrが生きていたら
        if (auto sp = w.lock())
        {
            //消す
            m_scene->m_DeleteObjects.push_back(sp);
        }
    }
        
    for (auto& w : m_spawnedCircles)
    {
        //今このptrが生きていたら
        if (auto sp = w.lock())
        {
            //消す
            m_scene->m_DeleteObjects.push_back(sp);
        }
    }

    //空にする
    m_spawnedPatrols.clear();
    m_spawnedCircles.clear();
}
