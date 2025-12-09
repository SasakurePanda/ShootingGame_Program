#include "Reticle.h"
#include "Input.h"
#include "Renderer.h"
#include "Application.h"
#include <Windows.h>
#include <iostream>

Reticle::Reticle(const std::wstring& texturePath, float size)
    : m_texturePath(texturePath), m_size(size)
{
    m_pos = Vector2(0.0f, 0.0f);
}

void Reticle::Initialize()
{
    // 自分のコンポーネントとして TextureComponent を作る
    m_texture = AddComponent<TextureComponent>();
    if (!m_texture)
    {
        OutputDebugStringA("Reticle: TextureComponent 作成失敗\n");
        return;
    }

    // テクスチャ読み込み（ワーキングディレクトリに依存）
    if (!m_texture->LoadTexture(m_texturePath))
    {
        OutputDebugStringA("Reticle: テクスチャ読み込み失敗\n");
    }
    m_texture->SetSize(m_size, m_size);

    // 初期位置：ウィンドウ中央
    RECT rc{};
    GetClientRect(Application::GetWindow(), &rc);
    m_pos.x = static_cast<float>((rc.right - rc.left) / 2);
    m_pos.y = static_cast<float>((rc.bottom - rc.top) / 2);

    // TextureComponent は SetScreenPosition が左上基準なのでここで設定しておく
    float left = m_pos.x - m_size * 0.5f;
    float top = m_pos.y - m_size * 0.5f;
    m_texture->SetScreenPosition(left, top);
}

void Reticle::Update(float dt)
{
    // マウス入力でドラッグ処理（Input::Update() は GameScene 側で呼んでいる想定）
    // 押した瞬間にドラッグ開始
    if (Input::IsMouseLeftPressed())
    {
        m_isDragging = true;
    }

    // 押している間は追従
    if (m_isDragging && Input::IsMouseLeftDown())
    {
        POINT p = Input::GetMousePosition(); // クライアント座標
        m_pos.x = static_cast<float>(p.x);
        m_pos.y = static_cast<float>(p.y);

        // TextureComponent は左上基準なので補正して渡す
        float left = m_pos.x - m_size * 0.5f;
        float top = m_pos.y - m_size * 0.5f;
        if (m_texture) m_texture->SetScreenPosition(left, top);
    }

    // 離したら固定してドラッグ終了
    if (!Input::IsMouseLeftDown() && m_isDragging)
    {
        m_isDragging = false;
        // 最終位置は m_pos のまま（置いたまま）
    }
}

void Reticle::Draw(float alpha)
{
    //Renderer::DrawReticle は中心座標の POINT を期待する実装を想定
    POINT center{ static_cast<LONG>(m_pos.x), static_cast<LONG>(m_pos.y) };
    Vector2 size(m_size, m_size);

    //DrawReticle 内で深度・ブレンドの切り替えを行い、
    //DrawTexture 側で SRV のアンバインドとシェーダ復帰を行うことを期待
    Renderer::DrawReticle(m_texture->GetSRV(), center, size);
}
