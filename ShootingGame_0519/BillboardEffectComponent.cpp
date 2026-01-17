#include "BillboardEffectComponent.h"
#include "TextureManager.h"
#include "GameObject.h"
#include "Renderer.h"
#include <algorithm>

void BillboardEffectComponent::SetConfig(const BillboardEffectConfig& config)
{
    m_texturePath = config.texturePath;
    m_size = config.size;
    m_duration = config.duration;
    m_isAdditive = config.isAdditive;
    m_cols = config.cols;
    m_rows = config.rows;
	m_color = config.color;
}

void BillboardEffectComponent::SetTexture(const std::string& path)
{
    m_texturePath = path;
    m_textureSrv  = nullptr; // Initialize‚ÅLoad‚·‚é
}

void BillboardEffectComponent::SetPartition(int cols, int rows)
{
    if (cols <= 0)
    {
        m_cols = 1;
    }
    else
    {
        m_cols = cols;
    }
    if (rows <= 0)
    {
        m_rows = 1;
    }
    else
    {
        m_rows = rows;
    }
}

void BillboardEffectComponent::Initialize()
{
    m_elapsed = 0.0f;
    m_isFinished = false;

    if (!m_texturePath.empty())
    {
        m_textureSrv = TextureManager::Load(m_texturePath);
    }
}

void BillboardEffectComponent::Update(float dt)
{
    if (dt <= 0.0f){ return; }

    if (m_isFinished){ return; }

    m_elapsed += dt;

    if (m_duration <= 0.0f)
    {
        m_isFinished = true;
        return;
    }

    if (m_elapsed >= m_duration)
    {
        m_elapsed = m_duration;
        m_isFinished = true;
    }
}

int BillboardEffectComponent::GetFrameIndex() const
{
    int frameCount = m_cols * m_rows;
    if (frameCount <= 1)
    {
        return 0;
    }

    if (m_duration <= 0.0f)
    {
        return frameCount - 1;
    }

    float t = std::clamp(m_elapsed / m_duration, 0.0f, 1.0f);
    int frame = (int)(t * (float)frameCount);

    if (frame >= frameCount)
    {
        frame = frameCount - 1;
    }

    return frame;
}

void BillboardEffectComponent::Draw(float dt)
{
    if (m_isFinished){ return; }

    if (!m_textureSrv){ return; }

    auto owner = GetOwner();
    if (!owner){ return; }

    const auto& pos = owner->GetPosition();
    int frameIndex = GetFrameIndex();

    DirectX::SimpleMath::Vector4 color(1, 1, 1, 1);

    Renderer::DrawBillboard(m_textureSrv,
                            pos,
                            m_size,
                            color,
                            m_cols,
                            m_rows,
                            frameIndex,
                            m_isAdditive);
}