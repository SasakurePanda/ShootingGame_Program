#include "TextureComponent.h"
#include "Renderer.h"
#include <WICTextureLoader.h>
#include "Application.h"
#include <iostream>

using namespace DirectX;
using namespace DirectX::SimpleMath;

TextureComponent::TextureComponent()
{
    m_Position = { 0.0f, 0.0f };
    m_Size = { 100.0f, 100.0f }; // èâä˙ÉTÉCÉY
}

bool TextureComponent::LoadTexture(const std::wstring& filepath)
{
    ID3D11Device* device = Renderer::GetDevice();
    if (!device) { return false; }

    HRESULT hr = CreateWICTextureFromFile(device, filepath.c_str(), nullptr, m_TextureSRV.GetAddressOf());
    return SUCCEEDED(hr);
}

void TextureComponent::Initialize() {}

void TextureComponent::Draw(float alpha)
{
    if (!m_TextureSRV) { return; }

    //std::cout << "[TextureComp] Draw start SRV=" << m_TextureSRV.Get() << std::endl;

    Renderer::DrawTexture(m_TextureSRV.Get(), m_Position, m_Size);

    //std::cout << "[TextureComp] Draw end\n";
}
