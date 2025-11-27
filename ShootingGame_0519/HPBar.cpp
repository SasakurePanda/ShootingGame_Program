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
}

void HPBar::Initialize()
{
    // TextureComponent を自分のコンポーネントとして作る
    m_frameTex = AddComponent<TextureComponent>();
    m_gaugeTex = AddComponent<TextureComponent>();

    if (!m_frameTex || !m_gaugeTex)
    {
        OutputDebugStringA("HPBar: TextureComponent 作成失敗\n");
        return;
    }

    if (!m_frameTex->LoadTexture(m_framePath))
    {
        OutputDebugStringA("HPBar: frame 読込失敗\n");
    }

    if (!m_gaugeTex->LoadTexture(m_gaugePath))
    {
        OutputDebugStringA("HPBar: gauge 読込失敗\n");
    }

    // 初期レイアウト反映
    UpdateLayout();
}

void HPBar::SetHP(float current, float max)
{
    m_currentHP = current;
    if (max > 0.0f) 
    {
        m_maxHP = max;
    }
    else 
    {
        m_maxHP = 1.0f;
    }

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
    if (m_frameTex)
    {
        m_frameTex->SetScreenPosition(m_pos.x, m_pos.y);
        m_frameTex->SetSize(m_width, m_height);
    }

    if (m_gaugeTex)
    {
        float gaugeW = m_width;
        float gaugeH = m_height * m_ratio;

        float gaugeX = m_pos.x;
        float gaugeY = m_pos.y;

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