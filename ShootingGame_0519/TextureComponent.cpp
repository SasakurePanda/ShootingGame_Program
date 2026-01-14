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
    m_Size = { 100.0f, 100.0f }; // 初期サイズ
}

bool TextureComponent::LoadTexture(const std::wstring& filepath)
{
    ID3D11Device* device = Renderer::GetDevice();
    if (!device) { return false; }

    HRESULT hr = CreateWICTextureFromFile(device, filepath.c_str(), nullptr, m_TextureSRV.GetAddressOf());
    return SUCCEEDED(hr);
}

void TextureComponent::Initialize() 
{

}

void TextureComponent::Draw(float deltaTime)
{

    if (!m_IsVisible)  { return; }
    if (!m_TextureSRV) { return; }

    Renderer::SetBlendState(BS_ALPHABLEND);
    Renderer::SetDepthEnable(false);

    // アルファ定数バッファを更新してバインド
    Renderer::SetTextureAlpha(m_Alpha);

    // 既存のテクスチャ描画（DrawTexture は SRV を使う）
    Renderer::DrawTexture(m_TextureSRV.Get(), m_Position, m_Size);

    // （DrawTexture 側で SRV のアンバインドやシェーダ復帰を行っているなら不要）
}
