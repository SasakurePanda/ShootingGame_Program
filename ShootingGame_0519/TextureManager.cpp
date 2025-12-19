// TextureManager.cpp
#ifdef _WIN32
#include <windows.h>
#endif
#include "TextureManager.h"
#include <WICTextureLoader.h>
#include "Renderer.h"

std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>> TextureManager::m_textures;

ID3D11ShaderResourceView* TextureManager::Load(const std::string& filepath)
{
    auto it = m_textures.find(filepath);
    if (it != m_textures.end())
    {
        return it->second.Get();
    }

    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> texture;
    HRESULT hr = DirectX::CreateWICTextureFromFile(
        Renderer::GetDevice(), std::wstring(filepath.begin(), filepath.end()).c_str(),
        nullptr, texture.GetAddressOf());

    if (SUCCEEDED(hr))
    {
        m_textures[filepath] = texture;
        return texture.Get();
    }

    return nullptr;
}