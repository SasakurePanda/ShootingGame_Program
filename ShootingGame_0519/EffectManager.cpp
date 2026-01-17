#include "EffectManager.h"
#include "GameObject.h"
#include "BillboardEffectComponent.h"
#include "renderer.h"

std::vector<std::shared_ptr<GameObject>> EffectManager::m_effectObjects;

void EffectManager::Init()
{
	m_effectObjects.clear();
}

void EffectManager::Update(float dt)
{
	for (auto& effect : m_effectObjects)
	{
		if (effect)
		{
			effect->Update(dt);
		}
	}
	RemoveFinishedEffects();
}

void EffectManager::Draw3D(float dt)
{
	for (auto& obj : m_effectObjects)
	{
		if (obj)
		{
			obj->Draw(dt);
		}
	}
}

void EffectManager::Uninit()
{
	m_effectObjects.clear();
}

void EffectManager::SpawnBillboardEffect(const BillboardEffectConfig& config,
								         const DirectX::SimpleMath::Vector3& pos)
{
	auto effe = std::make_shared<GameObject>();
	effe->SetPosition(pos);

	//ビルボードのエフェクトコンポーネントを追加
	auto fx = std::make_shared<BillboardEffectComponent>();
	fx->SetConfig(config);
	effe->AddComponent(fx);

	effe->Initialize();
	m_effectObjects.push_back(effe);
}

void EffectManager::SpawnExplosion(const DirectX::SimpleMath::Vector3& pos)
{
	BillboardEffectConfig config{};
	config.texturePath = "Asset/Effect/Effect_Explosion02.png"; 
	config.size = 24.0f;
	config.duration = 0.35f;
	config.cols = 3;
	config.rows = 3;
	config.isAdditive = true;
	config.color = DirectX::SimpleMath::Vector4(1, 1, 1, 1);

	SpawnBillboardEffect(config, pos);
}

void EffectManager::RemoveFinishedEffects()
{
	m_effectObjects.erase(
		std::remove_if(m_effectObjects.begin(), m_effectObjects.end(),
			[](const std::shared_ptr<GameObject>& obj)
			{
				if (!obj)
				{
					return true;
				}

				auto fx = obj->GetComponent<BillboardEffectComponent>();
				if (!fx)
				{
					return true;
				}

				return fx->IsFinished();
			}),
		m_effectObjects.end());
}

