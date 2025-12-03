#include "HPBar.h"
#include "Input.h"
#include "Renderer.h"
#include "Application.h"
#include <Windows.h>

HPBar::HPBar(const std::wstring& framePath, const std::wstring& gaugePath, float width, float height)
    : m_framePath(framePath)
    , m_gaugePath(gaugePath)
    , m_width(width)
    , m_height(height)
{
	//コンポーネントの初期化は初期化内で行う
}

void HPBar::Initialize()
{
	//ゲージと枠のテクスチャコンポーネントを追加
    m_frameTex = AddComponent<TextureComponent>();
    m_gaugeTex = AddComponent<TextureComponent>();

    if (!m_frameTex || !m_gaugeTex)
    {
        OutputDebugStringA("HPBar: TextureComponent 作成失敗\n");
        return;
    }

    if (!m_frameTex->LoadTexture(m_framePath))
    {
        OutputDebugStringA("HPBar: 枠の画像 読込失敗\n");
    }

    if (!m_gaugeTex->LoadTexture(m_gaugePath))
    {
        OutputDebugStringA("HPBar: ゲージの画像 読込失敗\n");
    }

    //初期レイアウト反映
    UpdateLayout();
}

void HPBar::SetHP(float current, float max)
{
	//HP更新
    m_currentHP = current;
    if (max > 0.0f) 
    {
        m_maxHP = max;
    }
    else 
    {
        m_maxHP = 1.0f;
    }

	//ゲージの目標比率更新
    m_targetRatio = std::clamp(m_currentHP / m_maxHP, 0.0f, 1.0f);
}

void HPBar::Update(float dt)
{
    if (std::abs(m_ratio - m_targetRatio) > 1e-4f)
    {
        float t = std::clamp(m_lerpSpeed * dt, 0.0f, 1.0f);
        m_ratio = m_ratio + (m_targetRatio - m_ratio) * t;
        UpdateLayout();
    }
}

void HPBar::UpdateLayout()
{
	//枠とゲージの位置・サイズ更新
    if (m_frameTex)
    {
        m_frameTex->SetScreenPosition(m_pos.x, m_pos.y);
        m_frameTex->SetSize(m_width, m_height);
    }

	//ゲージのサイズはHP残量に応じて変化
    if (m_gaugeTex)
    {
        // 現在の比率に合わせたゲージの高さ (縦縮小)
        float gaugeW = m_width;
        float gaugeH = m_height * m_ratio;
        //m_pos は左上基準（簡単）
        //「上から減らす」→ 上端を固定して高さだけ減らす（top-left をそのまま使う）
        // 左上基準なので、下端を合わせるには Y を下にシフトする
        float gaugeX = m_pos.x;
        float gaugeY = m_pos.y + (m_height - gaugeH);

        m_gaugeTex->SetScreenPosition(gaugeX, gaugeY);
        m_gaugeTex->SetSize(gaugeW, gaugeH);
    }
}

void HPBar::Draw(float alpha)
{
    if (m_gaugeTex && m_gaugeTex->GetSRV())
    {
        m_gaugeTex->Draw(alpha);
    }
    if (m_frameTex && m_frameTex->GetSRV())
    {
        m_frameTex->Draw(alpha);
    }
}