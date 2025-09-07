#pragma once
#include "GameObject.h"
#include "TextureComponent.h"
#include <SimpleMath.h>
#include <memory>
using namespace DirectX::SimpleMath;

class Reticle : public GameObject
{
public:
    // texturePath : ワイド文字列パス（LoadTexture は wstring を受け取る）
    Reticle(const std::wstring& texturePath, float size = 64.0f);
    ~Reticle() override = default;

    void Initialize() override;
    void Update(float dt) override;
    void Draw(float alpha) override;

    // サイズのセット（ピクセル）
    void SetSize(float size) { m_size = size; if (m_texture) m_texture->SetSize(m_size, m_size); }

private:
    std::wstring m_texturePath;
    float m_size;                 // 表示サイズ（幅＝高さ）
    Vector2 m_pos;                // 中心座標（画面クライアント座標、px）
    bool m_isDragging = false;

    // テクスチャコンポーネント（shared_ptr で管理）
    std::shared_ptr<TextureComponent> m_texture;
};

