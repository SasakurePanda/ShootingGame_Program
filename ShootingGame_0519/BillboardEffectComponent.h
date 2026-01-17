#pragma once
#include "Component.h"
#include <d3d11.h>
#include <string>
#include <SimpleMath.h>

/// <summary>
/// ビルボードでのエフェクトを出す際の情報の構造体
/// </summary>
struct BillboardEffectConfig
{
    float elapsed = 0.0f;
    float duration = 0.35f;
    bool  isAlive = true;
    DirectX::SimpleMath::Vector3 m_position = DirectX::SimpleMath::Vector3::Zero;
    float size = 8.0f;
    int cols = 4;
    int rows = 4;
    bool isAdditive = true;
    DirectX::SimpleMath::Vector4 color = DirectX::SimpleMath::Vector4(1, 1, 1, 1);
    std::string texturePath;
};


class BillboardEffectComponent : public Component
{
public:
    BillboardEffectComponent() = default;
    ~BillboardEffectComponent() override = default;

    void Initialize() override;
    void Update(float dt) override;
    void Draw(float dt) override;

    //----------Set関数-------------
    void SetConfig(const BillboardEffectConfig& config);

    void SetTexture(const std::string& path);
    void SetSize(float size) { m_size = size; }
    void SetDuration(float duration) { m_duration = duration; }
    void SetBlendAdditive(bool add) { m_isAdditive = add; }
    void SetPartition(int cols, int rows);
    void SetColor(const DirectX::SimpleMath::Vector4& color) { m_color = color; }
    //----------Get関数-------------
    bool IsFinished() const { return m_isFinished; }

private:

    int GetFrameIndex() const;

    //--------------寿命関連------------------
    float m_elapsed = 0.0f;
    float m_duration = 0.3f;
    bool m_isFinished = false;

    //--------------見た目関連------------------
    float m_size = 5.0f;
    bool m_isAdditive = true;

    int m_cols = 1;
    int m_rows = 1;
    
    std::string m_texturePath;

    DirectX::SimpleMath::Vector4 m_color = DirectX::SimpleMath::Vector4(1, 1, 1, 1);

 
    ID3D11ShaderResourceView* m_textureSrv = nullptr;
};

