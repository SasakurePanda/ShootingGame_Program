#include "GameObject.h"

void GameObject::Initialize()
{
    for (auto& comp : m_components)
    {
        comp->Initialize();
    }
}

void GameObject::Update(float dt)
{
    m_prevTransform = m_transform;

    // 各コンポーネントへ固定ステップ dt を渡す
    for (auto& comp : m_components) {
        comp->Update(dt);
    }
}

void GameObject::Draw(float alpha)
{
    for (auto& comp : m_components)
    {
        comp->Draw(alpha);
    }
}

void GameObject::AddComponent(std::shared_ptr<Component> comp) 
{
    comp->SetOwner(this);
    m_components.push_back(comp);
}
