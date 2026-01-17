#pragma once
#include <memory>
#include <vector>
#include <SimpleMath.h>

struct BillboardEffectConfig;
class GameObject;

class EffectManager
{
public:
    static void Init();
    static void Update(float dt);
    static void Draw3D(float dt);
    static void Uninit();

    //----------Spawnä÷êî-------------
    static void SpawnBillboardEffect(const BillboardEffectConfig& config, const DirectX::SimpleMath::Vector3& pos);
    static void SpawnExplosion(const DirectX::SimpleMath::Vector3& pos);
    //static void SpawnSmoke(const DirectX::SimpleMath::Vector3& pos, float size);
    //static void SpawnBulletTrail(const DirectX::SimpleMath::Vector3& from, const DirectX::SimpleMath::Vector3& to);

private:

	static std::vector<std::shared_ptr<GameObject>> m_effectObjects;
	static void RemoveFinishedEffects();
};
