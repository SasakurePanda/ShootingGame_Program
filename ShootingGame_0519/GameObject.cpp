#include "GameObject.h"

void GameObject::Initialize()
{
    for (auto& comp : m_components)
    {
        comp->Initialize();
    }
}

void GameObject::Update() 
{
    for (auto& comp : m_components) 
    {
        comp->Update();
    }
}

void GameObject::Draw() 
{
    for (auto& comp : m_components) 
    {
        comp->Draw();
    }
}

void GameObject::AddComponent(std::shared_ptr<Component> comp) 
{
    comp->SetOwner(this);
    m_components.push_back(comp);
}
