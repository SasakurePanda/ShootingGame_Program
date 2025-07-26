#include "ModelComponent.h"

#include "GameObject.h"
#include <iostream>

bool ModelComponent::LoadModel(const std::string& filepath)
{
    m_model = std::make_unique<Model>();

    if (!m_model->LoadFromFile(filepath))
    {
        std::cout << "ModelComponent: ƒ‚ƒfƒ‹‚Ì“Ç‚Ýž‚Ý‚ÉŽ¸”s‚µ‚Ü‚µ‚½F" << filepath << "\n";
        return false;
    }

    return true;
}

void ModelComponent::Draw()
{
    if (!m_model || !m_owner) return;

    const SRT& srt = m_owner->GetTransform(); // GameObject‚©‚çŽæ“¾
    m_model->Draw(srt);
}