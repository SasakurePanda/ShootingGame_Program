#include "BuildingSpawner.h"
#include "GameObject.h"
#include "GameScene.h"
#include "ModelComponent.h"
#include "BoxComponent.h"
#include "OBBColliderComponent.h"
#include "ModelCache.h"
#include "Building.h"
#include <random>
#include <cmath>

BuildingSpawner::BuildingSpawner(GameScene* scene) : m_scene(scene)
{
	std::random_device rd;
	m_rng.seed(rd());   //ランダムシードの初期化
}

static float RandFloatStd(std::mt19937_64& rng, float a, float b)
{
    std::uniform_real_distribution<float> d(a, b);
    return d(rng);
}

int BuildingSpawner::RandomSpawn(const BuildingConfig& cfg)
{
    if (!m_scene) { return 0; }

	m_placed.clear();       //配置済み建物情報のクリア

	float halfAreaX = cfg.areaWidth * 0.5f;     //エリアの半分の幅
	float halfAreaZ = cfg.areaDepth * 0.5f;     //エリアの半分の奥行き   
    
	int placedCount = 0;    //配置した建物の数保存用

    for (int i = 0; i < cfg.count; ++i)
    {
        bool placed = false;

		//Maxの試行回数まで試行
        for (int attempt = 0; attempt < cfg.maxAttemptsPerBuilding; ++attempt)
        {
            //候補位置・スケール・回転を作る
            float x = RandFloatStd(m_rng, -halfAreaX, halfAreaX);
            float z = RandFloatStd(m_rng, -halfAreaZ, halfAreaZ);

			//スケールをランダムに決定
            float scale = (cfg.minScale == cfg.maxScale) ? cfg.minScale : RandFloatStd(m_rng, cfg.minScale, cfg.maxScale);

			//Y軸回転をランダムに決定
            //float yaw = cfg.randomizeRotation ? RandFloatStd(m_rng, 0.0f, 2.0f * 3.14159265358979323846f) : 0.0f;

            //footprint（XZ）の半幅を計算（スケールをかける）
            float halfW = (cfg.footprintSizeX * 0.5f) * scale + cfg.spacing * 0.5f;
            float halfD = (cfg.footprintSizeZ * 0.5f) * scale + cfg.spacing * 0.5f;

            //既に配置したものと矩形衝突しないかチェック（XZ平面）
            bool overlap = false;
            for (const auto& pr : m_placed)
            {
                if (std::fabs(x - pr.cx) < (halfW + pr.halfW) &&
                    std::fabs(z - pr.cz) < (halfD + pr.halfD))
                {
                    overlap = true;
                    break;
                }
            }

            if (overlap) { continue; }  //衝突なら別の候補を試す

            // 衝突しなければ実際にオブジェクトを作成してシーンに追加する
            auto obj = std::make_shared<Building>();
            obj->SetScene(m_scene);
            //高さ（Y）は必要に応じて調整してください。ここでは -12 を元のコードと同じにしています
            obj->SetPosition({ x, -12.0f, z });
            obj->SetScale({ 10.0f, 10.0f, 10.0f });
            obj->SetRotation({ 0.0f, 0.0f, 0.0f });

            //コライダー（OBB）を追加。baseColliderSize に scale を乗算
            auto col = std::make_shared<AABBColliderComponent>();
            col->SetSize({ 3.5f, 16.5f, 3.5f });
            col->SetEnabled(false);
            obj->AddComponent(col);
            col->isStatic = true;

            //モデルを読み込みる
            auto mc = std::make_shared<ModelComponent>();
            mc->LoadModel(cfg.modelPath);
            obj->AddComponent(mc);

            obj->Initialize();
            m_scene->AddObject(obj);

            //記録
            PlacedRect pr;
            pr.cx = x;
            pr.cz = z;
            pr.halfW = halfW;
            pr.halfD = halfD;
            m_placed.push_back(pr);

            placed = true;
            ++placedCount;
            break;
        } //attempts

        if (!placed)
        {
            //配置失敗（試行回数上限）: ログ出しなどして続行
            char buf[256];
            sprintf_s(buf, "WARN: BuildingSpawner failed to place building %d (attempts=%d)\n", i, cfg.maxAttemptsPerBuilding);
            OutputDebugStringA(buf);
            //無理に置かず次へ進む
        }
    } 

    return placedCount;

}

int BuildingSpawner::Spawn(const BuildingConfig& cfg)
{
    if (!m_scene) { return 0; }

    m_placed.clear();   // ひとまずこの呼び出しで配置した矩形だけを見る

    // 置く数は fixedPositions の数と count の小さい方
    int numPositions = static_cast<int>(cfg.fixedPositions.size());
    if (numPositions <= 0) { return 0; }

    int numToSpawn = numPositions;
    if (cfg.count > 0 && cfg.count < numToSpawn)
    {
        numToSpawn = cfg.count;
    }

    int placedCount = 0;

    for (int i = 0; i < numToSpawn; ++i)
    {
        DirectX::SimpleMath::Vector3 pos = cfg.fixedPositions[i];

        float x = pos.x;
        float z = pos.z;

        // footprint（XZ）の半幅を計算（spacing も足す）
        float halfW = (cfg.footprintSizeX * 0.5f) + cfg.spacing * 0.5f;
        float halfD = (cfg.footprintSizeZ * 0.5f) + cfg.spacing * 0.5f;

        // 既に配置したものと矩形衝突しないかチェック（XZ平面）
        bool overlap = false;
        for (const auto& pr : m_placed)
        {
            if (std::fabs(x - pr.cx) < (halfW + pr.halfW) &&
                std::fabs(z - pr.cz) < (halfD + pr.halfD))
            {
                overlap = true;
                break;
            }
        }

        if (overlap)
        {
            // ここでは「重なりそうならスキップ」としておく
            // 必要ならログだけ出して強制配置、などに変えても OK
            char buf[256];
            sprintf_s(buf, "WARN: BuildingSpawner::Spawn overlap at index %d, skip.\n", i);
            OutputDebugStringA(buf);
            continue;
        }

        // 実際のオブジェクト生成処理（RandomSpawn と合わせておく）
        auto obj = std::make_shared<Building>();
        obj->SetScene(m_scene);

        // Y は fixedPositions の y をそのまま使う
        // これまで通りにしたいなら pos.y の代わりに -12.0f に固定しても良いです
        obj->SetPosition({ x, pos.y, z });

        // ★今は固定値ですが、必要なら cfg.minScale / maxScale から計算しても OK
        obj->SetScale({ 10.0f, 10.0f, 10.0f });
        obj->SetRotation({ 0.0f, 0.0f, 0.0f });

        // コライダー（AABB）を追加
        auto col = std::make_shared<AABBColliderComponent>();
        col->SetSize({ 3.5f, 16.5f, 3.5f });
        col->SetEnabled(false);
        obj->AddComponent(col);
        col->isStatic = true;

        // モデルを読み込む
        auto mc = std::make_shared<ModelComponent>();
        mc->LoadModel(cfg.modelPath);
        obj->AddComponent(mc);

        obj->Initialize();
        m_scene->AddObject(obj);

        // footprint を記録
        PlacedRect pr;
        pr.cx = x;
        pr.cz = z;
        pr.halfW = halfW;
        pr.halfD = halfD;
        m_placed.push_back(pr);

        ++placedCount;
    }

    return placedCount;
}
